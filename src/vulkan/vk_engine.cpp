#include "vk_engine.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include <VkBootstrap.h>
#include <glm/gtx/transform.hpp>
#include <vulkan/vk_enum_string_helper.h>

#define SOURCE_LOCATION __builtin_FILE() << ":" << __builtin_LINE() << " (" << __builtin_FUNCTION() << ")"

#define VK_CHECK(x)                                                                                           \
    do {                                                                                                      \
        VkResult err = x;                                                                                     \
        if (err) {                                                                                            \
            std::cerr << SOURCE_LOCATION << ": Detected Vulkan error: " << string_VkResult(err) << std::endl; \
            std::abort();                                                                                     \
        }                                                                                                     \
    } while (0)

namespace dfv {

    void VulkanEngine::init() {
        // Load core Vulkan structures
        initVulkan();

        // Create the swapchain
        initSwapchain();
        // Create the default renderpass
        initDefaultRenderpass();
        // Create framebuffers
        initFramebuffers();

        // Create descriptors
        initDescriptors();
        // Create pipelines
        initPipelines();

        // Create the command buffers
        initCommands();

        // Creates synchronization structures
        initSyncStructures();

        isInitialized = true;
    }

    void VulkanEngine::draw() {
        auto &frame = getCurrentFrame();
        VkCommandBuffer cmd = frame.mainCommandBuffer;

        // Wait until the GPU has finished rendering the last frame, 1 second timeout
        VK_CHECK(vkWaitForFences(device, 1, &frame.renderFence, true, 1000000000));
        VK_CHECK(vkResetFences(device, 1, &frame.renderFence));

        // Request an image from the swapchain, 1 second timeout
        uint32_t swapchainImageIndex;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, frame.presentSemaphore, nullptr, &swapchainImageIndex));

        // Now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again
        VK_CHECK(vkResetCommandBuffer(frame.mainCommandBuffer, 0));

        // Begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
        const VkCommandBufferBeginInfo cmdBeginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                                       .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        // Clear color to sky blue
        const VkClearValue colorClear = {.color = {{0.39f, 0.58f, 0.93f, 1.0f}}};

        // Clear depth
        const VkClearValue depthClear = {.depthStencil = {.depth = 1.0f}};

        std::array clearValues = {colorClear, depthClear};

        // We will use the clear color from above, and the framebuffer of the index the swapchain gave us
        const VkRenderPassBeginInfo rpInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                              .renderPass = renderPass,
                                              .framebuffer = framebuffers[swapchainImageIndex],
                                              .renderArea = {.extent = windowExtent},
                                              .clearValueCount = clearValues.size(),
                                              .pClearValues = clearValues.data()};

        const VkViewport viewport = {.x = 0.0f,
                                     .y = 0.0f,
                                     .width = static_cast<float>(windowExtent.width),
                                     .height = static_cast<float>(windowExtent.height),
                                     .minDepth = 0.0f,
                                     .maxDepth = 1.0f};

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &rpInfo.renderArea);

        drawObjects(cmd);

        vkCmdEndRenderPass(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        // We want to wait on the presentSemaphore, as that semaphore is signaled when the swapchain is ready
        // We will signal the renderSemaphore, to signal that rendering has finished
        const VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                     .waitSemaphoreCount = 1,
                                     .pWaitSemaphores = &frame.presentSemaphore,
                                     .pWaitDstStageMask = &waitStage,
                                     .commandBufferCount = 1,
                                     .pCommandBuffers = &cmd,
                                     .signalSemaphoreCount = 1,
                                     .pSignalSemaphores = &frame.renderSemaphore};

        // renderFence will now block until the graphic commands finish execution
        VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, frame.renderFence));

        // We want to wait on the renderSemaphore before presentation
        // as it's necessary that drawing commands have finished before the image is displayed to the user
        const VkPresentInfoKHR presentInfo = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                              .waitSemaphoreCount = 1,
                                              .pWaitSemaphores = &frame.renderSemaphore,
                                              .swapchainCount = 1,
                                              .pSwapchains = &swapchain,
                                              .pImageIndices = &swapchainImageIndex};

        const auto result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            recreateSwapchain();
        else
            VK_CHECK(result);

        frameNumber++;
    }

    void VulkanEngine::drawObjects(VkCommandBuffer cmdBuf) {
        const auto &frame = getCurrentFrame();

        // Camera view
        const glm::mat4 view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
        // Camera projection
        glm::mat4 projection = glm::perspective(camera.fov,
                                                static_cast<float>(windowExtent.width) / static_cast<float>(windowExtent.height),
                                                camera.nearPlane, camera.farPlane);
        projection[1][1] *= -1; // Flip projection matrix from GL to Vulkan orientation (y-axis)

        // Precompute the view-projection matrix
        const glm::mat4 viewProj = projection * view;

        // Update scene parameters
        const float framed = static_cast<float>(frameNumber) / 120.f;
        sceneParameters.ambientColor = {sin(framed), 0, cos(framed), 1};
        sceneParameters.eyePos = camera.position;
        sceneParameters.sunlightDirection = glm::vec3(cos(glm::radians(135.0f)) * cos(glm::radians(210.0f)), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(glm::radians(210.0f)));
        //white sunlight color white
        sceneParameters.sunlightColor = {1, 1, 1, 1};

        std::byte *sceneBuffer;
        vmaMapMemory(allocator, sceneParametersBuffer.allocation, reinterpret_cast<void **>(&sceneBuffer));

        const unsigned int frameIndex = frameNumber % MaxFramesInFlight;
        sceneBuffer += uniformBufferSizeAlignUp(sizeof(uniform::SceneData)) * frameIndex;
        std::memcpy(sceneBuffer, &sceneParameters, sizeof(uniform::SceneData));

        vmaUnmapMemory(allocator, sceneParametersBuffer.allocation);

        // Keep track of the last used mesh and material to avoid unnecessary binding
        const Mesh *lastMesh = nullptr;
        const Material *lastMaterial = nullptr;
        for (auto &object : renderObjects) {
            const uint32_t objectIndex{static_cast<uint32_t>(std::distance(renderObjects.data(), &object))};

            // Bind the pipeline if it doesn't match with the already bound one
            if (object.material != lastMaterial) {
                vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
                lastMaterial = object.material;

                // Bind descriptor set 0 (global data, camera and scene)
                uint32_t uniformOffset = uniformBufferSizeAlignUp(sizeof(uniform::SceneData)) * frameIndex;
                vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1,
                                        &frame.globalDescriptor, 1, &uniformOffset);

                // Bind descriptor set 1 (object data)
                vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1,
                                        &frame.objectDescriptor, 0, nullptr);

                // Bind the texture descriptor if the material has one
                if (object.material->textureSet != VK_NULL_HANDLE) {
                    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 2, 1,
                                            &object.material->textureSet, 0, nullptr);
                }
            }

            // Upload the model transform matrix to the GPU via push constants
            uniform::MeshPushConstants constants = {.modelTransform = object.transform,
                                                    .worldTransform = viewProj * object.transform};

            vkCmdPushConstants(cmdBuf, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(uniform::MeshPushConstants), &constants);

            // Only bind the mesh if it's a different one from last bind
            if (object.mesh != lastMesh) {
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(cmdBuf, 0, 1, &object.mesh->vertexBuffer.buffer, &offset);
                vkCmdBindIndexBuffer(cmdBuf, object.mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                lastMesh = object.mesh;
            }

            vkCmdDrawIndexed(cmdBuf, object.mesh->indices.size(), 1, 0, 0, objectIndex);
        }
    }

    void VulkanEngine::cleanup() {
        if (!isInitialized)
            return;

        std::array<VkFence, MaxFramesInFlight> fences = {};
        std::ranges::transform(frames, fences.begin(), [](auto &frame) {
            return frame.renderFence;
        });
        vkWaitForFences(device, fences.size(), fences.data(), true, 1000000000);

        mainDeletionQueue.flush();
        swapchainDeletionQueue.flush();

        vmaDestroyAllocator(allocator);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkb::destroy_debug_utils_messenger(instance, debugMessenger);
        vkDestroyInstance(instance, nullptr);
    }

    FrameData &VulkanEngine::getCurrentFrame() {
        return frames[frameNumber % MaxFramesInFlight];
    }

} // namespace dfv
