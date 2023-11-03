#include "vk_engine.h"

#include <cstdlib>
#include <iostream>

#include <VkBootstrap.h>
#include <glm/gtx/transform.hpp>
#include <vk_mem_alloc.h>
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

    void VulkanEngine::init(GLFWwindow *window, uint32_t width, uint32_t height) {
        windowExtent.width = width;
        windowExtent.height = height;

        // Load core Vulkan structures
        initVulkan(window);
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

        // Initialize the placeholder scene
        loadMeshes();
        initScene();

        isInitialized = true;
    }

    void VulkanEngine::loadMeshes() {
        createMesh("monkey", "assets/monkey_smooth.obj");
    }

    void VulkanEngine::initScene() {
        Material *defaultMeshMaterial = getMaterial("defaultmesh");
        assert(defaultMeshMaterial != nullptr);

        RenderObject monkey = {};
        monkey.mesh = getMesh("monkey");
        assert(monkey.mesh != nullptr);
        monkey.material = defaultMeshMaterial;
        monkey.position = {0, 0, 0};
        monkey.orientation = {0, 0, 0};
        monkey.scale = {1, 1, 1};

        monkey.updateFunc = [this](RenderObject &object, seconds_f deltaTime) {
            object.position = glm::vec3{2 * sin(frameNumber / 120.f),
                                        0,
                                        2 * cos(frameNumber / 120.f)};
            object.orientation += glm::vec3{0, 1, 0} * deltaTime.count();
            object.computeTransform();
        };

        renderObjects.push_back(monkey);

        Mesh *triangleMesh = getMesh("monkey");
        assert(triangleMesh != nullptr);
        // Add a bunch of extra triangles around the monkey
        for (int x = -20; x <= 20; x++) {
            for (int y = -20; y <= 20; y++) {
                RenderObject triangle = {};
                triangle.mesh = triangleMesh;
                triangle.material = defaultMeshMaterial;
                triangle.position = {x, 0, y};
                triangle.orientation = {0, 0, 0};
                triangle.scale = {0.2, 0.2, 0.2};

                triangle.updateFunc = [&](RenderObject &object, seconds_f deltaTime) {
                    object.orientation += glm::vec3{0, -1, 0} * deltaTime.count();
                    object.computeTransform();
                };

                renderObjects.push_back(triangle);
            }
        }

        // Initialize camera parameters
        cameraParameters.position = {0.f, -6.f, -10.f};
        cameraParameters.orientation = {0.f, 0.f, 0.f};
        cameraParameters.fov = glm::radians(70.f);
        cameraParameters.nearPlane = 0.1f;
        cameraParameters.farPlane = 200.f;
        cameraParameters.movementSpeed = 5.f;
        cameraParameters.rotationSpeed = glm::radians(30.f);
    }

    void VulkanEngine::drawObjects(VkCommandBuffer cmdBuf) {
        auto &frame = getCurrentFrame();

        // Apply camera position
        glm::mat4 view = glm::translate(glm::mat4(1.f), cameraParameters.position);
        // Apply pitch rotation
        glm::mat4 rot = glm::rotate(glm::mat4(1.f), cameraParameters.orientation.x, glm::vec3(-1.f, 0.f, 0.f));
        // Apply yaw rotation
        rot = glm::rotate(rot, cameraParameters.orientation.y, glm::vec3(0.f, 1.f, 0.f));
        view = rot * view;

        // Camera projection
        glm::mat4 projection = glm::perspective(cameraParameters.fov,
                                                (float) windowExtent.width / (float) windowExtent.height,
                                                cameraParameters.nearPlane, cameraParameters.farPlane);
        projection[1][1] *= -1;

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
            objectBuffer++->modelMatrix = object.transformMatrix;
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
            constants.renderMatrix = object.transformMatrix;

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

    void VulkanEngine::draw() {
        auto &frame = getCurrentFrame();

        // Wait until the GPU has finished rendering the last frame, 1 second timeout
        VK_CHECK(vkWaitForFences(device, 1, &frame.renderFence, true, 1000000000));
        VK_CHECK(vkResetFences(device, 1, &frame.renderFence));

        // Request an image from the swapchain, 1 second timeout
        uint32_t swapchainImageIndex;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, frame.presentSemaphore, nullptr, &swapchainImageIndex));

        // Now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again
        VK_CHECK(vkResetCommandBuffer(frame.mainCommandBuffer, 0));

        VkCommandBuffer cmd = frame.mainCommandBuffer;

        // Begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
        VkCommandBufferBeginInfo cmdBeginInfo = {};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;

        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

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
        VkRenderPassBeginInfo rpInfo = {};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.pNext = nullptr;

        rpInfo.renderPass = renderPass;
        rpInfo.renderArea.offset.x = 0;
        rpInfo.renderArea.offset.y = 0;
        rpInfo.renderArea.extent = windowExtent;
        rpInfo.framebuffer = framebuffers[swapchainImageIndex];

        // Set clear values
        rpInfo.clearValueCount = clearValues.size();
        rpInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        drawObjects(cmd);

        // Finalize the render pass
        vkCmdEndRenderPass(cmd);
        // Finalize the command buffer (we can no longer add commands, but it can now be executed)
        VK_CHECK(vkEndCommandBuffer(cmd));

        // Prepare the submission to the queue.
        // We want to wait on the presentSemaphore, as that semaphore is signaled when the swapchain is ready
        // We will signal the renderSemaphore, to signal that rendering has finished
        VkSubmitInfo submit = {};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.pNext = nullptr;

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        submit.pWaitDstStageMask = &waitStage;

        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &frame.presentSemaphore;

        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &frame.renderSemaphore;

        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;

        // Submit command buffer to the queue and execute it.
        // renderFence will now block until the graphic commands finish execution
        VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, frame.renderFence));

        // This will put the image we just rendered into the visible window
        // We want to wait on the renderSemaphore for that,
        // as it's necessary that drawing commands have finished before the image is displayed to the user
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;

        presentInfo.pSwapchains = &swapchain;
        presentInfo.swapchainCount = 1;

        presentInfo.pWaitSemaphores = &frame.renderSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &swapchainImageIndex;

        VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

        // Increase the number of frames drawn
        frameNumber++;
    }

    void VulkanEngine::update(seconds_f deltaTime) {
        for (auto &object : renderObjects) {
            if (object.updateFunc)
                object.updateFunc(object, deltaTime);
        }

        updateCamera(deltaTime);
    }

    void VulkanEngine::updateCamera(seconds_f deltaTime) {
        glm::vec3 cameraOrientation = {cameraParameters.pitchDirection.load(std::memory_order_relaxed),
                                       cameraParameters.yawDirection.load(std::memory_order_relaxed),
                                       0.f};

        // Multiply by delta time for framerate-independent movement
        cameraParameters.orientation += cameraOrientation * cameraParameters.rotationSpeed * deltaTime.count();

        glm::vec3 directions = {cameraParameters.surgeDirection.load(std::memory_order_relaxed), // Forward/backward
                                cameraParameters.heaveDirection.load(std::memory_order_relaxed), // Up/down
                                cameraParameters.swayDirection.load(std::memory_order_relaxed)}; // Left/right
        float yaw = cameraParameters.orientation.y;
        glm::vec3 cameraMovement = {directions.x * sin(yaw) + directions.z * cos(yaw),
                                    directions.y,
                                    directions.z * sin(yaw) - directions.x * cos(yaw)};

        cameraParameters.position -= cameraMovement * cameraParameters.adjustedMovementSpeed() * deltaTime.count();
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
