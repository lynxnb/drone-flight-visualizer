#include "vk_engine.h"

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <VkBootstrap.h>
#include <glm/gtx/transform.hpp>
#include <tiny_obj_loader.h>
#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>

#include "vk_initializers.h"
#include "vk_pipeline.h"
#include "vk_types.h"
#include <config/config.h>

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

        initDescriptors();

        // Create pipelines
        initPipelines();

        loadMeshes();

        initScene();

        isInitialized = true;
    }

    void VulkanEngine::initVulkan(GLFWwindow *window) {
        vkb::InstanceBuilder builder;

        // Make the Vulkan instance with basic debug features
        auto instanceResult = builder.set_app_name(window_config::WindowTitle)
                                      .request_validation_layers(true)
                                      .require_api_version(1, 1, 0)
                                      .use_default_debug_messenger()
                                      .build();

        vkb::Instance vkbInstance = instanceResult.value();

        instance = vkbInstance.instance;
        debugMessenger = vkbInstance.debug_messenger;

        // Get the surface of the window we opened with GLFW
        VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

        // Use vkbootstrap to select a GPU.
        // We want a GPU that can write to the GLFW surface and supports Vulkan 1.1
        // Automatically picks the dedicated GPU if available
        vkb::PhysicalDeviceSelector selector{vkbInstance};
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

        // Use vulkan functions already retrieved by vkbootstrap for VMA
        auto vkFunctions = VmaVulkanFunctions{
                .vkGetInstanceProcAddr = vkbInstance.fp_vkGetInstanceProcAddr,
                .vkGetDeviceProcAddr = vkbInstance.fp_vkGetDeviceProcAddr,
        };

        // Initialize the memory allocator
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = chosenGPU;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        allocatorInfo.pVulkanFunctions = &vkFunctions;

        vmaCreateAllocator(&allocatorInfo, &allocator);

        std::construct_at(&traits, vkbDevice.physical_device.properties);
        std::cout << traits.summary() << std::endl;
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

        // Enqueue the destruction of the swapchain
        mainDeletionQueue.pushFunction([=, this]() {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        });

        // Depth image size will match the window
        VkExtent3D depthImageExtent = {windowExtent.width, windowExtent.height, 1};

        // Set the depth format to 32-bit float
        depthFormat = VK_FORMAT_D32_SFLOAT;

        // The depth image will be an image with the format we selected and depth attachment usage flag
        VkImageCreateInfo depthImageInfo = vkinit::image_create_info(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

        // For the depth image, we want to allocate it from GPU local memory
        VmaAllocationCreateInfo depthImageAllocInfo = {};
        depthImageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        depthImageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // Allocate and create the image
        VK_CHECK(vmaCreateImage(allocator, &depthImageInfo, &depthImageAllocInfo, &depthImage.image, &depthImage.allocation, nullptr));

        //build an image-view for the depth image to use for rendering
        VkImageViewCreateInfo depthImageViewInfo = vkinit::imageview_create_info(depthFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

        VK_CHECK(vkCreateImageView(device, &depthImageViewInfo, nullptr, &depthImageView));

        mainDeletionQueue.pushFunction([=, this]() {
            vkDestroyImageView(device, depthImageView, nullptr);
            vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
        });
    }

    void VulkanEngine::initCommands() {
        // Require the pool to allow for resetting of individual command buffers
        VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily,
                                                                                   VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        // Create a command pool per frame, with one command buffer in it
        for (auto &frame : frames) {
            VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frame.commandPool));

            // Allocate the default command buffer that we will use for rendering
            VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frame.commandPool, 1);

            VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frame.mainCommandBuffer));

            // Enqueue the destruction of the pool
            mainDeletionQueue.pushFunction([=, this]() {
                vkDestroyCommandPool(device, frame.commandPool, nullptr);
            });
        }
    }

    void VulkanEngine::initSyncStructures() {
        // We want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
        VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

        for (auto &frame : frames) {
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frame.renderFence));

            VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frame.presentSemaphore));
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frame.renderSemaphore));

            // Enqueue the destruction of the fence and semaphores
            mainDeletionQueue.pushFunction([=, this]() {
                vkDestroyFence(device, frame.renderFence, nullptr);
                vkDestroySemaphore(device, frame.presentSemaphore, nullptr);
                vkDestroySemaphore(device, frame.renderSemaphore, nullptr);
            });
        }
    }

    void VulkanEngine::initDefaultRenderpass() {
        // Allocate memory for the attachments
        std::array<VkAttachmentDescription, 2> attachments = {};

        VkAttachmentDescription &colorAttachment = attachments[0]; // The renderpass will use this color attachment.
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

        // Depth attachment
        VkAttachmentDescription &depthAttachment = attachments[1];
        depthAttachment.flags = 0;
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Create 1 subpass, which is the minimum you can do
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        // Allocate memory for the subpass dependencies
        std::array<VkSubpassDependency, 2> dependencies = {};

        // Color dependency
        VkSubpassDependency &colorDependency = dependencies[0];
        colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        colorDependency.dstSubpass = 0;
        colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.srcAccessMask = 0;
        colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Depth dependency
        VkSubpassDependency &depthDependency = dependencies[1];
        depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        depthDependency.dstSubpass = 0;
        depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depthDependency.srcAccessMask = 0;
        depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        // Set the color/depth attachments
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        // Set the subpass
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        // Set the subpass dependencies
        renderPassInfo.dependencyCount = dependencies.size();
        renderPassInfo.pDependencies = dependencies.data();

        VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

        // Enqueue the destruction of the renderpass
        mainDeletionQueue.pushFunction([=, this]() {
            vkDestroyRenderPass(device, renderPass, nullptr);
        });
    }

    void VulkanEngine::initFramebuffers() {
        // Create the framebuffers for the swapchain images. This will connect the renderpass to the images for rendering
        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.pNext = nullptr;

        fbInfo.renderPass = renderPass;
        fbInfo.width = windowExtent.width;
        fbInfo.height = windowExtent.height;
        fbInfo.layers = 1;

        // Grab how many images we have in the swapchain
        const uint32_t swapchainImageCount = swapchainImages.size();
        framebuffers = std::vector<VkFramebuffer>(swapchainImageCount);

        // Create framebuffers for each of the swapchain image views
        for (int i = 0; i < swapchainImageCount; i++) {
            // Allocate memory for the attachments
            std::array<VkImageView, 2> attachments = {swapchainImageViews[i], depthImageView};
            fbInfo.attachmentCount = attachments.size();
            fbInfo.pAttachments = attachments.data();

            VK_CHECK(vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]));

            // Enqueue the destruction of the framebuffers
            mainDeletionQueue.pushFunction([=, this]() {
                vkDestroyFramebuffer(device, framebuffers[i], nullptr);
                vkDestroyImageView(device, swapchainImageViews[i], nullptr);
            });
        }
    }

    void VulkanEngine::initDescriptors() {
        // Create a descriptor pool that will hold 10 uniform buffers
        VkDescriptorPoolSize poolSizes = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10};

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = 0;
        poolInfo.maxSets = 10;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSizes;

        vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};
        // Binding for camera data at 0
        bindings[0] = vkinit::descriptorset_layout_binding(
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

        // Binding for scene data at 1
        bindings[1] = vkinit::descriptorset_layout_binding(
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);

        VkDescriptorSetLayoutCreateInfo setInfo = {};
        setInfo.bindingCount = bindings.size();
        setInfo.flags = 0;
        setInfo.pNext = nullptr;
        setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        setInfo.pBindings = bindings.data();

        vkCreateDescriptorSetLayout(device, &setInfo, nullptr, &globalSetLayout);

        const size_t sceneParamBufferSize = MaxFramesInFlight * uniformBufferSizeAlignUp(sizeof(SceneData));
        sceneParametersBuffer = createBuffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        for (int i = 0; i < frames.size(); i++) {
            FrameData &frame = frames[i];
            // Allocate the camera buffer
            frame.cameraBuffer = createBuffer(sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            // Add to the deletion queue
            mainDeletionQueue.pushFunction([&]() {
                vmaDestroyBuffer(allocator, frame.cameraBuffer.buffer, frame.cameraBuffer.allocation);
            });

            // Allocate one descriptor set for each frame
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.pNext = nullptr;
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            // using the pool we just created
            allocInfo.descriptorPool = descriptorPool;
            allocInfo.descriptorSetCount = 1;
            // using the global data layout
            allocInfo.pSetLayouts = &globalSetLayout;

            vkAllocateDescriptorSets(device, &allocInfo, &frame.globalDescriptor);

            VkDescriptorBufferInfo cameraInfo;
            cameraInfo.buffer = frame.cameraBuffer.buffer;
            cameraInfo.offset = 0;
            cameraInfo.range = sizeof(CameraData);

            VkDescriptorBufferInfo sceneInfo;
            sceneInfo.buffer = sceneParametersBuffer.buffer;
            sceneInfo.offset = uniformBufferSizeAlignUp(sizeof(SceneData)) * i;
            sceneInfo.range = sizeof(SceneData);

            std::array<VkWriteDescriptorSet, 2> setWrites = {};
            // Camera write
            setWrites[0] = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frame.globalDescriptor, &cameraInfo, 0);
            // Scene write
            setWrites[1] = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frame.globalDescriptor, &sceneInfo, 1);

            vkUpdateDescriptorSets(device, setWrites.size(), setWrites.data(), 0, nullptr);
        }

        mainDeletionQueue.pushFunction([=, this]() {
            vmaDestroyBuffer(allocator, sceneParametersBuffer.buffer, sceneParametersBuffer.allocation);

            vkDestroyDescriptorSetLayout(device, globalSetLayout, nullptr);
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        });
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
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipeline_layout_create_info();
        VkPipelineLayout trianglePipelineLayout;

        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &trianglePipelineLayout));

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

        // Default depth testing
        pipelineBuilder.depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

        // Use the triangle layout we created
        pipelineBuilder.pipelineLayout = trianglePipelineLayout;

        // Finally build the pipeline
        VkPipeline trianglePipeline = pipelineBuilder.buildPipeline(device, renderPass);

        // Now create the red triangle pipeline, only change shaders but keep the rest of the pipeline settings
        pipelineBuilder.shaderStages.clear();
        pipelineBuilder.shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, redTriangleVertShader));
        pipelineBuilder.shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, redTriangleFragShader));
        VkPipeline redTrianglePipeline = pipelineBuilder.buildPipeline(device, renderPass);

        VkPipelineLayoutCreateInfo meshPipelineLayoutInfo = vkinit::pipeline_layout_create_info();

        // Setup push constants
        VkPushConstantRange pushConstant;
        // This push constant range starts at the beginning
        pushConstant.offset = 0;
        // This push constant range takes up the size of a MeshPushConstants struct
        pushConstant.size = sizeof(MeshPushConstants);
        // This push constant range is accessible only in the vertex shader
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        meshPipelineLayoutInfo.pPushConstantRanges = &pushConstant;
        meshPipelineLayoutInfo.pushConstantRangeCount = 1;

        meshPipelineLayoutInfo.pSetLayouts = &globalSetLayout;
        meshPipelineLayoutInfo.setLayoutCount = 1;

        VkPipelineLayout meshPipelineLayout;
        VK_CHECK(vkCreatePipelineLayout(device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout));

        // Build the mesh pipeline
        VertexInputDescription vertexDescription = Vertex::getVertexDescription();

        // Connect the pipeline builder vertex input info to the one we get from Vertex
        pipelineBuilder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
        pipelineBuilder.vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

        pipelineBuilder.vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
        pipelineBuilder.vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

        // Clear the shader stages for the builder
        pipelineBuilder.shaderStages.clear();

        // Compile mesh vertex shader
        VkShaderModule meshVertShader;
        if (!loadShaderModule("shaders/tri_mesh.vert.spv", &meshVertShader))
            std::cout << "Error when building the triangle vertex shader module" << std::endl;
        else
            std::cout << "Mesh Triangle vertex shader successfully loaded" << std::endl;

        VkShaderModule colorMeshShader;
        if (!loadShaderModule("shaders/default_lit.frag.spv", &colorMeshShader))
            std::cout << "Error when building the colored mesh shader" << std::endl;
        else
            std::cout << "Colored mesh shader successfully loaded" << std::endl;

        // Add the other shaders
        pipelineBuilder.shaderStages.push_back(
                vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));

        // Use default_lit shader
        pipelineBuilder.shaderStages.push_back(
                vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, colorMeshShader));

        // Use the push constants layout
        pipelineBuilder.pipelineLayout = meshPipelineLayout;
        // Build the mesh triangle pipeline
        VkPipeline meshPipeline = pipelineBuilder.buildPipeline(device, renderPass);

        createMaterial("defaultmesh", meshPipeline, meshPipelineLayout);

        // Delete all the vulkan shaders
        vkDestroyShaderModule(device, meshVertShader, nullptr);
        vkDestroyShaderModule(device, redTriangleVertShader, nullptr);
        vkDestroyShaderModule(device, redTriangleFragShader, nullptr);
        vkDestroyShaderModule(device, triangleFragShader, nullptr);
        vkDestroyShaderModule(device, triangleVertexShader, nullptr);
        vkDestroyShaderModule(device, colorMeshShader, nullptr);

        // Add the pipelines to the deletion queue
        mainDeletionQueue.pushFunction([=, this]() {
            vkDestroyPipeline(device, redTrianglePipeline, nullptr);
            vkDestroyPipeline(device, trianglePipeline, nullptr);
            vkDestroyPipeline(device, meshPipeline, nullptr);

            vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);
            vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
        });
    }

    void VulkanEngine::loadMeshes() {
        // Make the array 3 vertices long
        Mesh triangleMesh = {}, monkeyMesh = {};

        triangleMesh.vertices.resize(3);

        // Vertex positions
        triangleMesh.vertices[0].position = {1.f, 1.f, 0.0f};
        triangleMesh.vertices[1].position = {-1.f, 1.f, 0.0f};
        triangleMesh.vertices[2].position = {0.f, -1.f, 0.0f};

        // Vertex colors, all green
        triangleMesh.vertices[0].color = {0.f, 1.f, 0.0f};
        triangleMesh.vertices[1].color = {0.f, 1.f, 0.0f};
        triangleMesh.vertices[2].color = {0.f, 1.f, 0.0f};

        // Load the monkey
        if (!monkeyMesh.loadFromObj("assets/monkey_smooth.obj"))
            throw std::runtime_error("Failed to load the monkey mesh");

        uploadMesh(triangleMesh);
        uploadMesh(monkeyMesh);

        // Make a copy of these. Eventually we will delete the hardcoded monkey and triangle meshes, so it's no problem now.
        meshes["monkey"] = monkeyMesh;
        meshes["triangle"] = triangleMesh;
    }

    void VulkanEngine::uploadMesh(dfv::Mesh &mesh) {
        // Allocate vertex buffer
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        // The total size, in bytes, of the buffer we are allocating
        bufferInfo.size = mesh.vertices.size() * sizeof(Vertex);
        // This buffer is going to be used as a Vertex Buffer
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        // Let the VMA library know that this data should be writeable by CPU, but also readable by GPU
        VmaAllocationCreateInfo vmaAllocInfo = {};
        vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        // Allocate the buffer
        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo,
                                 &mesh.vertexBuffer.buffer,
                                 &mesh.vertexBuffer.allocation,
                                 nullptr));

        // Add the destruction of this buffer to the deletion queue
        mainDeletionQueue.pushFunction([=, this]() {
            vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
        });

        // Copy vertex data
        void *data;
        vmaMapMemory(allocator, mesh.vertexBuffer.allocation, &data);

        std::memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));

        vmaUnmapMemory(allocator, mesh.vertexBuffer.allocation);
    }

    void VulkanEngine::initScene() {
        Material *defaultMeshMaterial = getMaterial("defaultmesh");

        RenderObject monkey = {};
        monkey.mesh = getMesh("monkey");
        monkey.material = defaultMeshMaterial;
        monkey.transformMatrix = glm::mat4{1.0f};

        renderables.push_back(monkey);

        Mesh *triangleMesh = getMesh("triangle");
        // Add a bunch of extra triangles around the monkey
        for (int x = -20; x <= 20; x++) {
            for (int y = -20; y <= 20; y++) {
                RenderObject triangle = {};
                triangle.mesh = triangleMesh;
                triangle.material = defaultMeshMaterial;
                glm::mat4 translation = glm::translate(glm::mat4{1.0}, glm::vec3(x, 0, y));
                glm::mat4 scale = glm::scale(glm::mat4{1.0}, glm::vec3(0.2, 0.2, 0.2));
                triangle.transformMatrix = translation * scale;

                renderables.push_back(triangle);
            }
        }
    }

    Material *VulkanEngine::createMaterial(const std::string &name, VkPipeline pipeline, VkPipelineLayout layout) {
        Material material = {};
        material.pipeline = pipeline;
        material.pipelineLayout = layout;

        auto [materialIt, isNewInsertion] = materials.insert_or_assign(name, material);
        if (!isNewInsertion)
            std::cerr << "Warning: material '" << name << "' was overwritten" << std::endl;

        return &materialIt->second;
    }

    Material *VulkanEngine::getMaterial(const std::string &name) {
        auto it = materials.find(name);
        if (it == materials.end())
            return nullptr;
        else
            return &it->second;
    }

    Mesh *VulkanEngine::getMesh(const std::string &name) {
        auto it = meshes.find(name);
        if (it == meshes.end())
            return nullptr;
        else
            return &(*it).second;
    }

    void VulkanEngine::drawObjects(VkCommandBuffer cmdBuf, std::span<RenderObject> objects) {
        auto &frame = getCurrentFrame();

        // Make a model view matrix for rendering the object camera view
        glm::vec3 camPos = {0.f, -6.f, -10.f};

        glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
        // Camera projection
        glm::mat4 projection = glm::perspective(glm::radians(70.f), (float) windowExtent.width / (float) windowExtent.height, 0.1f, 200.0f);
        projection[1][1] *= -1;

        // Fill the camera data struct
        CameraData camData = {};
        camData.proj = projection;
        camData.view = view;
        camData.viewproj = projection * view;

        // Copy the camera data into the buffer
        void *data;
        vmaMapMemory(allocator, frame.cameraBuffer.allocation, &data);
        std::memcpy(data, &camData, sizeof(CameraData));
        vmaUnmapMemory(allocator, frame.cameraBuffer.allocation);

        float framed = static_cast<float>(frameNumber) / 120.f;
        sceneParameters.ambientColor = {sin(framed), 0, cos(framed), 1};

        std::byte *sceneBuffer;
        vmaMapMemory(allocator, sceneParametersBuffer.allocation, reinterpret_cast<void **>(&sceneBuffer));

        unsigned int frameIndex = frameNumber % MaxFramesInFlight;
        sceneBuffer += uniformBufferSizeAlignUp(sizeof(SceneData)) * frameIndex;
        std::memcpy(sceneBuffer, &sceneParameters, sizeof(SceneData));

        vmaUnmapMemory(allocator, sceneParametersBuffer.allocation);

        // Keep track of the last used mesh and material to avoid unnecessary binding
        Mesh *lastMesh = nullptr;
        Material *lastMaterial = nullptr;
        for (auto &object : objects) {
            // Bind the pipeline if it doesn't match with the already bound one
            if (object.material != lastMaterial) {
                vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
                lastMaterial = object.material;
                vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1,
                                        &frame.globalDescriptor, 0, nullptr);
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

            vkCmdDraw(cmdBuf, object.mesh->vertices.size(), 1, 0, 0);
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

        drawObjects(cmd, renderables);

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

    void VulkanEngine::cleanup() {
        if (isInitialized) {
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
    }

    bool VulkanEngine::loadShaderModule(const std::filesystem::path &filePath, VkShaderModule *outShaderModule) const {
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

    AllocatedBuffer VulkanEngine::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const {
        // Allocate the vertex buffer
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.size = allocSize;
        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaAllocInfo = {};
        vmaAllocInfo.usage = memoryUsage;

        AllocatedBuffer newBuffer = {};

        //allocate the buffer
        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation,
                                 nullptr));

        return newBuffer;
    }

    FrameData &VulkanEngine::getCurrentFrame() {
        return frames[frameNumber % MaxFramesInFlight];
    }

    size_t VulkanEngine::uniformBufferSizeAlignUp(size_t size) const {
        // Calculate required alignment based on minimum device offset alignment
        size_t minUboAlignment = traits.minUniformBufferOffsetAlignment;
        if (minUboAlignment > 0)
            size = (size + minUboAlignment - 1) & ~(minUboAlignment - 1);

        return size;
    }

} // namespace dfv
