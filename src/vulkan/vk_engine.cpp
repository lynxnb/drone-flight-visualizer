﻿#include "vk_engine.h"

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

    void VulkanEngine::init(SurfaceWrapper &surfaceWrap) {
        windowExtent = surfaceWrap.getExtent();

        // Load core Vulkan structures
        initVulkan(surfaceWrap);
        // Create the swapchain
        initSwapchain();
        // Create the command buffers
        initCommands();
        // Creates synchronization structures
        initSyncStructures();
        // Create the default renderpass
        initDefaultRenderpass();
        // Create framebuffers
        initFramebuffers();
        // Create descriptors
        initDescriptors();
        // Create pipelines
        initPipelines();

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
        VkCommandBufferBeginInfo cmdBeginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                                 .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        // Allocate memory for the clear values
        std::array<VkClearValue, 2> clearValues = {};

        // Make a clear-color from frame number. This will flash with a 120*pi frame period.
        VkClearValue &colorClear = clearValues[0];
        float flash = abs(sin(static_cast<float>(frameNumber) / 120.f));
        colorClear.color = {
                {0.0f, 0.0f, flash, 1.0f}
        };

        // Clear depth
        VkClearValue &depthClear = clearValues[1];
        depthClear.depthStencil.depth = 1.0f;

        // Start the main renderpass
        // We will use the clear color from above, and the framebuffer of the index the swapchain gave us
        VkRenderPassBeginInfo rpInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                        .renderPass = renderPass,
                                        .framebuffer = framebuffers[swapchainImageIndex],
                                        .renderArea = {.extent = windowExtent},
                                        .clearValueCount = clearValues.size(),
                                        .pClearValues = clearValues.data()};

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        drawObjects(cmd);

        // Finalize the render pass
        vkCmdEndRenderPass(cmd);
        // Finalize the command buffer (we can no longer add commands, but it can now be executed)
        VK_CHECK(vkEndCommandBuffer(cmd));

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        // Prepare the submission to the queue
        // We want to wait on the presentSemaphore, as that semaphore is signaled when the swapchain is ready
        // We will signal the renderSemaphore, to signal that rendering has finished
        VkSubmitInfo submit = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                               .waitSemaphoreCount = 1,
                               .pWaitSemaphores = &frame.presentSemaphore,
                               .pWaitDstStageMask = &waitStage,
                               .commandBufferCount = 1,
                               .pCommandBuffers = &cmd,
                               .signalSemaphoreCount = 1,
                               .pSignalSemaphores = &frame.renderSemaphore};

        // Submit command buffer to the queue and execute it
        // renderFence will now block until the graphic commands finish execution
        VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, frame.renderFence));

        // This will put the image we just rendered into the visible window
        // We want to wait on the renderSemaphore for that,
        // as it's necessary that drawing commands have finished before the image is displayed to the user
        VkPresentInfoKHR presentInfo = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                        .waitSemaphoreCount = 1,
                                        .pWaitSemaphores = &frame.renderSemaphore,
                                        .swapchainCount = 1,
                                        .pSwapchains = &swapchain,
                                        .pImageIndices = &swapchainImageIndex};

        VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

        // Increase the number of frames drawn
        frameNumber++;
    }

    void VulkanEngine::drawObjects(VkCommandBuffer cmdBuf) {
        auto &frame = getCurrentFrame();

        // Camera view
        glm::mat4 view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
        // Camera projection
        glm::mat4 projection = glm::perspective(camera.fov,
                                                (float) windowExtent.width / (float) windowExtent.height,
                                                camera.nearPlane, camera.farPlane);
        projection[1][1] *= -1; // Flip projection matrix from GL to Vulkan orientation (y-axis)

        // Copy the camera data into the buffer
        CameraData *cameraData;
        vmaMapMemory(allocator, frame.cameraBuffer.allocation, reinterpret_cast<void **>(&cameraData));
        *cameraData = {.view = view,
                       .proj = projection,
                       .viewproj = projection * view};
        vmaUnmapMemory(allocator, frame.cameraBuffer.allocation);

        // Update scene parameters
        float framed = static_cast<float>(frameNumber) / 120.f;
        sceneParameters.ambientColor = {sin(framed), 0, cos(framed), 1};

        std::byte *sceneBuffer;
        vmaMapMemory(allocator, sceneParametersBuffer.allocation, reinterpret_cast<void **>(&sceneBuffer));

        unsigned int frameIndex = frameNumber % MaxFramesInFlight;
        sceneBuffer += uniformBufferSizeAlignUp(sizeof(SceneData)) * frameIndex;
        std::memcpy(sceneBuffer, &sceneParameters, sizeof(SceneData));

        vmaUnmapMemory(allocator, sceneParametersBuffer.allocation);

        // Update object data
        ObjectData *objectBuffer;
        vmaMapMemory(allocator, frame.objectBuffer.allocation, reinterpret_cast<void **>(&objectBuffer));
        for (auto &object : renderObjects)
            objectBuffer++->modelMatrix = object.transform;
        vmaUnmapMemory(allocator, frame.objectBuffer.allocation);

        // Keep track of the last used mesh and material to avoid unnecessary binding
        Mesh *lastMesh = nullptr;
        Material *lastMaterial = nullptr;
        for (auto &object : renderObjects) {
            uint32_t objectIndex{static_cast<uint32_t>(std::distance(renderObjects.data(), &object))};

            // Bind the pipeline if it doesn't match with the already bound one
            if (object.material != lastMaterial) {
                vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
                lastMaterial = object.material;

                // Bind descriptor set 0 (global data, camera and scene)
                uint32_t uniformOffset = uniformBufferSizeAlignUp(sizeof(SceneData)) * frameIndex;
                vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1,
                                        &frame.globalDescriptor, 1, &uniformOffset);

                // Bind descriptor set 1 (object data)
                vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1,
                                        &frame.objectDescriptor, 0, nullptr);
            }

            MeshPushConstants constants = {};
            // Final render matrix, that we are calculating on the cpu
            constants.renderMatrix = object.transform;

            // Upload the mesh render matrix to the GPU via push constants
            vkCmdPushConstants(cmdBuf, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

            // Only bind the mesh if it's a different one from last bind
            if (object.mesh != lastMesh) {
                // Bind the mesh vertex buffer with offset 0
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(cmdBuf, 0, 1, &object.mesh->vertexBuffer.buffer, &offset);
                lastMesh = object.mesh;
            }

            vkCmdDraw(cmdBuf, object.mesh->vertices.size(), 1, 0, objectIndex);
        }
    }

    void VulkanEngine::cleanup() {
        if (!isInitialized)
            return;

        std::array<VkFence, MaxFramesInFlight> fences = {};
        std::transform(frames.begin(), frames.end(), fences.begin(), [](auto &frame) {
            return frame.renderFence;
        });
        vkWaitForFences(device, fences.size(), fences.data(), true, 1000000000);

        mainDeletionQueue.flush();

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
