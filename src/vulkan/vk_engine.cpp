﻿#include "vk_engine.h"

#include <cstdlib>
#include <fstream>
#include <iostream>

#include "VkBootstrap.h"
#include "vk_initializers.h"
#include "vk_pipeline.h"
#include "vk_types.h"

#define SOURCE_LOCATION __builtin_FILE() << ":" << __builtin_LINE() << " (" << __builtin_FUNCTION() << ")"

#define VK_CHECK(x)                                                                          \
    do {                                                                                     \
        VkResult err = x;                                                                    \
        if (err) {                                                                           \
            std::cerr << SOURCE_LOCATION << ": Detected Vulkan error: " << err << std::endl; \
            std::abort();                                                                    \
        }                                                                                    \
    } while (0)

namespace dfv {

    void VulkanEngine::init() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow((int) windowExtent.width, (int) windowExtent.height, "drone_flight_visualizer",
                                  nullptr, nullptr);

        // Load core Vulkan structures
        initVulkan();

        // Create the swapchain
        initSwapchain();

        // Create the command buffer
        initCommands();

        // Create the default renderpass
        initDefaultRenderpass();

        // Create framebuffers
        initFramebuffers();

        // Creates synchronization structures
        initSyncStructures();

        // Create pipelines
        initPipelines();

        isInitialized = true;
    }

    void VulkanEngine::initVulkan() {
        vkb::InstanceBuilder builder;

        // Make the Vulkan instance with basic debug features
        auto inst_ret = builder.set_app_name("drone_flight_visualizer")
                                .request_validation_layers(true)
                                .require_api_version(1, 1, 0)
                                .use_default_debug_messenger()
                                .build();

        vkb::Instance vkb_inst = inst_ret.value();

        instance = vkb_inst.instance;
        debugMessenger = vkb_inst.debug_messenger;

        // Get the surface of the window we opened with GLFW
        VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

        // Use vkbootstrap to select a GPU.
        // We want a GPU that can write to the GLFW surface and supports Vulkan 1.1
        // Automatically picks the dedicated GPU if available
        vkb::PhysicalDeviceSelector selector{vkb_inst};
        vkb::PhysicalDevice physicalDevice = selector
                                                     .set_minimum_version(1, 1)
                                                     .set_surface(surface)
                                                     .select()
                                                     .value();

        // Create the final Vulkan device
        vkb::DeviceBuilder deviceBuilder{physicalDevice};

        vkb::Device vkbDevice = deviceBuilder.build().value();

        // Get the VkDevice handle used in the rest of a Vulkan application
        device = vkbDevice.device;
        chosenGPU = physicalDevice.physical_device;

        // Use vkbootstrap to get a Graphics queue
        graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    }

    void VulkanEngine::initSwapchain() {
        vkb::SwapchainBuilder swapchainBuilder{chosenGPU, device, surface};

        vkb::Swapchain vkbSwapchain = swapchainBuilder
                                              .use_default_format_selection()
                                              .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // Use vsync present mode
                                              .set_desired_extent(windowExtent.width, windowExtent.height)
                                              .build()
                                              .value();

        // Store the swapchain and its related images
        swapchain = vkbSwapchain.swapchain;
        swapchainImages = vkbSwapchain.get_images().value();
        swapchainImageViews = vkbSwapchain.get_image_views().value();

        swapchainImageFormat = vkbSwapchain.image_format;
    }

    void VulkanEngine::initCommands() {
        // Create a command pool for commands submitted to the graphics queue.
        // We also want the pool to allow for resetting of individual command buffers
        VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily,
                                                                                   VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool));

        // Allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &mainCommandBuffer));
    }

    void VulkanEngine::initDefaultRenderpass() {
        VkAttachmentDescription colorAttachment = {}; // The renderpass will use this color attachment.
        colorAttachment.format = swapchainImageFormat; // The attachment will have the format needed by the swapchain
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 1 sample, we won't be doing MSAA
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // We Clear when this attachment is loaded
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // We keep the attachment stored when the renderpass ends

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // We don't care about stencil
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // We don't know or care about the starting layout of the attachment
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // After the renderpass ends, the image has to be on a layout ready for display

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0; // Attachment number will index into the pAttachments array in the parent renderpass itself
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Create 1 subpass, which is the minimum you can do
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        // Set the color attachment
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        // Set the subpass
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
    }

    void VulkanEngine::initFramebuffers() {
        // Create the framebuffers for the swapchain images. This will connect the renderpass to the images for rendering
        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.pNext = nullptr;

        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.width = windowExtent.width;
        fbInfo.height = windowExtent.height;
        fbInfo.layers = 1;

        // Grab how many images we have in the swapchain
        const uint32_t swapchainImageCount = swapchainImages.size();
        framebuffers = std::vector<VkFramebuffer>(swapchainImageCount);

        // Create framebuffers for each of the swapchain image views
        for (int i = 0; i < swapchainImageCount; i++) {
            fbInfo.pAttachments = &swapchainImageViews[i];
            VK_CHECK(vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]));
        }
    }

    void VulkanEngine::initSyncStructures() {
        // Create synchronization structures
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = nullptr;

        // We want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFence));

        // For the semaphores we don't need any flags
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = nullptr;
        semaphoreCreateInfo.flags = 0;

        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentSemaphore));
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderSemaphore));
    }

    void VulkanEngine::initPipelines() {
        VkShaderModule triangleFragShader;
        if (!loadShaderModule("shaders/coloredTriangle.frag.spv", &triangleFragShader))
            std::cout << "Error when building the triangle fragment shader module" << std::endl;
        else
            std::cout << "Triangle fragment shader successfully loaded" << std::endl;

        VkShaderModule triangleVertexShader;
        if (!loadShaderModule("shaders/coloredTriangle.vert.spv", &triangleVertexShader))
            std::cout << "Error when building the triangle vertex shader module" << std::endl;
        else
            std::cout << "Triangle vertex shader successfully loaded" << std::endl;

        VkShaderModule redTriangleFragShader;
        if (!loadShaderModule("shaders/triangle.frag.spv", &redTriangleFragShader))
            std::cout << "Error when building the triangle fragment shader module" << std::endl;
        else
            std::cout << "Triangle fragment shader successfully loaded" << std::endl;

        VkShaderModule redTriangleVertShader;
        if (!loadShaderModule("shaders/triangle.vert.spv", &redTriangleVertShader))
            std::cout << "Error when building the triangle vertex shader module" << std::endl;
        else
            std::cout << "Triangle vertex shader successfully loaded" << std::endl;

        // Build the pipeline layout that controls the inputs/outputs of the shader
        // We are not using descriptor sets or other systems yet, so no need to use anything other than empty default
        VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();

        VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &trianglePipelineLayout));

        // Build the stage-create-info for both vertex and fragment stages
        PipelineBuilder pipelineBuilder;

        pipelineBuilder.shaderStages.push_back(
                vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));
        pipelineBuilder.shaderStages.push_back(
                vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

        // Vertex input controls how to read vertices from vertex buffers. We aren't using it yet
        pipelineBuilder.vertexInputInfo = vkinit::vertex_input_state_create_info();

        // Input assembly is the configuration for drawing triangle lists, strips, or individual points
        // We are just going to draw triangle list
        pipelineBuilder.inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        // Build viewport and scissor from the swapchain extents
        pipelineBuilder.viewport.x = 0.0f;
        pipelineBuilder.viewport.y = 0.0f;
        pipelineBuilder.viewport.width = (float) windowExtent.width;
        pipelineBuilder.viewport.height = (float) windowExtent.height;
        pipelineBuilder.viewport.minDepth = 0.0f;
        pipelineBuilder.viewport.maxDepth = 1.0f;

        pipelineBuilder.scissor.offset = {0, 0};
        pipelineBuilder.scissor.extent = windowExtent;

        // Configure the rasterizer to draw filled triangles
        pipelineBuilder.rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

        // We don't use multisampling, so just run the default one
        pipelineBuilder.multisampling = vkinit::multisampling_state_create_info();

        // A single blend attachment with no blending and writing to RGBA
        pipelineBuilder.colorBlendAttachment = vkinit::color_blend_attachment_state();

        // Use the triangle layout we created
        pipelineBuilder.pipelineLayout = trianglePipelineLayout;

        // Finally build the pipeline
        trianglePipeline = pipelineBuilder.build_pipeline(device, renderPass);

        // Now create the red triangle pipeline, only change shaders but keep the rest of the pipeline settings
        pipelineBuilder.shaderStages.clear();
        pipelineBuilder.shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, redTriangleVertShader));
        pipelineBuilder.shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));
        redTrianglePipeline = pipelineBuilder.build_pipeline(device, renderPass);
    }

    void VulkanEngine::draw() {
        // Wait until the GPU has finished rendering the last frame, 1 second timeout
        VK_CHECK(vkWaitForFences(device, 1, &renderFence, true, 1000000000));
        VK_CHECK(vkResetFences(device, 1, &renderFence));

        // Request an image from the swapchain, 1 second timeout
        uint32_t swapchainImageIndex;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, presentSemaphore, nullptr, &swapchainImageIndex));

        // Now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
        VK_CHECK(vkResetCommandBuffer(mainCommandBuffer, 0));

        VkCommandBuffer cmd = mainCommandBuffer;

        // Begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
        VkCommandBufferBeginInfo cmdBeginInfo = {};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.pNext = nullptr;

        cmdBeginInfo.pInheritanceInfo = nullptr;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        // Make a clear-color from frame number. This will flash with a 120*pi frame period.
        VkClearValue clearValue;
        float flash = abs(sin(static_cast<float>(frameNumber) / 120.f));
        clearValue.color = {{0.0f, 0.0f, flash, 1.0f}};

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
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (selectedShader == 0)
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
        else
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, redTrianglePipeline);
        vkCmdDraw(cmd, 3, 1, 0, 0);

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
        submit.pWaitSemaphores = &presentSemaphore;

        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &renderSemaphore;

        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;

        // Submit command buffer to the queue and execute it.
        // renderFence will now block until the graphic commands finish execution
        VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, renderFence));

        // This will put the image we just rendered into the visible window
        // We want to wait on the renderSemaphore for that,
        // as it's necessary that drawing commands have finished before the image is displayed to the user
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;

        presentInfo.pSwapchains = &swapchain;
        presentInfo.swapchainCount = 1;

        presentInfo.pWaitSemaphores = &renderSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &swapchainImageIndex;

        VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

        // Increase the number of frames drawn
        frameNumber++;
    }

    void VulkanEngine::run() {
        glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

        //main loop
        while (!glfwWindowShouldClose(window)) {
            //Handle events on queue
            glfwPollEvents();

            int state = glfwGetKey(window, GLFW_KEY_SPACE);
            if (state == GLFW_PRESS)
                selectedShader = (selectedShader + 1) % 2;

            draw();
        }
    }

    void VulkanEngine::cleanup() {
        if (isInitialized) {
            vkDestroyCommandPool(device, commandPool, nullptr);

            vkDestroySwapchainKHR(device, swapchain, nullptr);

            vkDestroyRenderPass(device, renderPass, nullptr);

            // Destroy swapchain resources
            for (int i = 0; i < framebuffers.size(); i++) {
                vkDestroyFramebuffer(device, framebuffers[i], nullptr);

                vkDestroyImageView(device, swapchainImageViews[i], nullptr);
            }

            vkDestroyDevice(device, nullptr);
            vkDestroySurfaceKHR(instance, surface, nullptr);
            vkb::destroy_debug_utils_messenger(instance, debugMessenger);
            vkDestroyInstance(instance, nullptr);
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }

    bool VulkanEngine::loadShaderModule(std::filesystem::path filePath, VkShaderModule *outShaderModule) {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
            return false;

        std::streamoff fileSize{file.tellg()};
        // Spirv expects the buffer to be on uint32, reserve an int vector big enough for the entire file
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char *>(buffer.data()), fileSize);

        file.close();

        // Create a new shader module, using the buffer we loaded
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        // codeSize has to be in bytes
        createInfo.codeSize = buffer.size() * sizeof(uint32_t);
        createInfo.pCode = buffer.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            return false;

        *outShaderModule = shaderModule;
        return true;
    }

} // namespace dfv