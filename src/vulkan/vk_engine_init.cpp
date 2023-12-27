#include "vk_engine.h"

#include <iostream>

#include <VkBootstrap.h>
#include <vulkan/vk_enum_string_helper.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "vk_initializers.h"
#include "vk_pipeline.h"

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

    void VulkanEngine::initVulkan() {
        vkb::InstanceBuilder builder;

        // Make the Vulkan instance with basic debug features
        auto instanceResult = builder.set_app_name("Vulkan Flight Visualizer")
                                      .request_validation_layers(true)
                                      .require_api_version(1, 1, 0)
                                      .use_default_debug_messenger()
                                      .build();

        vkb::Instance vkbInstance = instanceResult.value();

        instance = vkbInstance.instance;
        debugMessenger = vkbInstance.debug_messenger;

        // Get the surface of the window we opened with GLFW
        auto surfaceResult = surfaceWrap.getSurface(instance);
        VK_CHECK(surfaceResult.first);
        surface = surfaceResult.second;

        // Use vkbootstrap to select a GPU.
        // We want a GPU that can write to the GLFW surface and supports Vulkan 1.1
        // Automatically picks the dedicated GPU if available
        vkb::PhysicalDeviceSelector selector{vkbInstance};
        vkb::PhysicalDevice physicalDevice = selector
                                                     .set_minimum_version(1, 1)
                                                     .set_surface(surface)
                                                     .select()
                                                     .value();

        // Request the 'shader draw parameters' feature for gl_BaseInstance used in the vertex shader
        VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures =
                {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
                 .shaderDrawParameters = VK_TRUE};

        // Create the final Vulkan device
        vkb::DeviceBuilder deviceBuilder{physicalDevice};
        vkb::Device vkbDevice = deviceBuilder
                                        .add_pNext(&shaderDrawParametersFeatures)
                                        .build()
                                        .value();

        // Get the VkDevice handle used in the rest of a Vulkan application
        device = vkbDevice.device;
        chosenGPU = physicalDevice.physical_device;

        // Use vkbootstrap to get a Graphics queue
        graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        // Initialize the memory allocator
        VmaAllocatorCreateInfo allocatorInfo = {.physicalDevice = chosenGPU,
                                                .device = device,
                                                .instance = instance,
                                                .vulkanApiVersion = VK_API_VERSION_1_1};

        vmaCreateAllocator(&allocatorInfo, &allocator);

        std::construct_at(&traits, vkbDevice.physical_device.properties);
        std::cout << traits.summary() << std::endl;
    }

    void VulkanEngine::recreateSwapchain() {
        std::array<VkFence, MaxFramesInFlight> fences = {};
        std::ranges::transform(frames, fences.begin(), [](auto &frame) {
            return frame.renderFence;
        });
        vkWaitForFences(device, fences.size(), fences.data(), true, 1000000000);
        swapchainDeletionQueue.flush();

        initSwapchain();
        initFramebuffers();

        if (swapchainImageFormat != renderPassColorFormat)
            throw std::runtime_error("Swapchain recreation with different image formats is not supported");
    }

    void VulkanEngine::initSwapchain() {
        windowExtent = surfaceWrap.getExtent();

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

        // Enqueue the destruction of the swapchain, this will destruct images too
        swapchainDeletionQueue.pushFunction([=, this] {
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        });
    }

    void VulkanEngine::initDefaultRenderpass() {
        // The default renderpass will draw to the framebuffer image and use the depth image for depth testing

        // Set the depth format to 32-bit float
        depthFormat = VK_FORMAT_D32_SFLOAT;

        // The color attachment that describes the output image used by the render pass
        const VkAttachmentDescription colorAttachment = {.format = swapchainImageFormat, // The attachment will have the format needed by the swapchain
                                                         .samples = VK_SAMPLE_COUNT_1_BIT, // 1 sample, we won't be doing MSAA
                                                         .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // Clear the attachment when it is loaded
                                                         .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Store to the attachment when the renderpass ends
                                                         .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, // Ignore stencil, this is a color attachment only
                                                         .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                         .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // Don't care about the starting layout of the attachment
                                                         .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR}; // Request a layout ready for display at the end of the renderpass

        renderPassColorFormat = swapchainImageFormat;

        // The depth buffer will be a simple depth image
        const VkAttachmentDescription depthAttachment = {.format = depthFormat,
                                                         .samples = VK_SAMPLE_COUNT_1_BIT,
                                                         .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                         .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                                         .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                         .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                         .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                         .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        // Attachment number will index into the pAttachments array in the parent renderpass
        const VkAttachmentReference colorAttachmentRef = {.attachment = 0,
                                                          .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        const VkAttachmentReference depthAttachmentRef = {.attachment = 1,
                                                          .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        // Create a subpass that uses the above attachments
        const VkSubpassDescription subpass = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                              .colorAttachmentCount = 1,
                                              .pColorAttachments = &colorAttachmentRef,
                                              .pDepthStencilAttachment = &depthAttachmentRef};

        // Color dependency between color writes of the previous frame and writes of the current frame
        const VkSubpassDependency colorDependency = {.srcSubpass = VK_SUBPASS_EXTERNAL,
                                                     .dstSubpass = 0,
                                                     .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                     .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                     .srcAccessMask = 0,
                                                     .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};

        // Depth dependency between depth writes of the previous frame and writes of the current frame
        const VkSubpassDependency depthDependency = {.srcSubpass = VK_SUBPASS_EXTERNAL,
                                                     .dstSubpass = 0,
                                                     .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                                     .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                                     .srcAccessMask = 0,
                                                     .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};

        const std::array attachments = {colorAttachment, depthAttachment};
        const std::array dependencies = {colorDependency, depthDependency};

        const VkRenderPassCreateInfo renderPassInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                                       .attachmentCount = attachments.size(),
                                                       .pAttachments = attachments.data(),
                                                       .subpassCount = 1,
                                                       .pSubpasses = &subpass,
                                                       .dependencyCount = dependencies.size(),
                                                       .pDependencies = dependencies.data()};

        VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

        // Enqueue the destruction of the renderpass
        mainDeletionQueue.pushFunction([=, this] {
            vkDestroyRenderPass(device, renderPass, nullptr);
        });
    }

    void VulkanEngine::initFramebuffers() {
        // First create the depth image, the size will match the window
        const VkExtent3D depthImageExtent = {windowExtent.width, windowExtent.height, 1};

        const VkImageCreateInfo depthImageInfo = vkinit::image_create_info(
                depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

        // Allocate from GPU local memory
        const VmaAllocationCreateInfo depthImageAllocInfo = {.usage = VMA_MEMORY_USAGE_GPU_ONLY,
                                                             .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

        // Allocate and create the image
        VK_CHECK(vmaCreateImage(allocator, &depthImageInfo, &depthImageAllocInfo,
                                &depthImage.image, &depthImage.allocation, nullptr));

        const VkImageViewCreateInfo depthImageViewInfo = vkinit::imageview_create_info(depthFormat, depthImage.image,
                                                                                       VK_IMAGE_ASPECT_DEPTH_BIT);

        VK_CHECK(vkCreateImageView(device, &depthImageViewInfo, nullptr, &depthImageView));

        swapchainDeletionQueue.pushFunction([=, this] {
            vkDestroyImageView(device, depthImageView, nullptr);
            vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
        });

        // Create the framebuffers for the swapchain images. This will connect the renderpass to the images for rendering
        VkFramebufferCreateInfo framebufferInfo = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                                   .renderPass = renderPass,
                                                   .width = windowExtent.width,
                                                   .height = windowExtent.height,
                                                   .layers = 1};

        const uint32_t swapchainImageCount = swapchainImages.size();
        framebuffers = std::vector<VkFramebuffer>(swapchainImageCount);

        // Create framebuffers for each of the swapchain image views
        for (int i = 0; i < swapchainImageCount; i++) {
            std::array attachments = {swapchainImageViews[i], depthImageView};
            framebufferInfo.attachmentCount = attachments.size();
            framebufferInfo.pAttachments = attachments.data();

            VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]));

            // Enqueue the destruction of the framebuffers
            swapchainDeletionQueue.pushFunction([=, this] {
                vkDestroyFramebuffer(device, framebuffers[i], nullptr);
                vkDestroyImageView(device, swapchainImageViews[i], nullptr);
            });
        }
    }

    void VulkanEngine::initDescriptors() {
        std::array poolSizes = {
                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
                VkDescriptorPoolSize{        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10}
        };

        const VkDescriptorPoolCreateInfo poolInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                     .flags = 0,
                                                     .maxSets = 10,
                                                     .poolSizeCount = poolSizes.size(),
                                                     .pPoolSizes = poolSizes.data()};

        vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

        // Set 0: Global data
        const VkDescriptorSetLayoutBinding sceneBinding = vkinit::descriptorset_layout_binding(
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

        std::array globalBindings = {sceneBinding};
        const VkDescriptorSetLayoutCreateInfo globalSetInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                               .flags = 0,
                                                               .bindingCount = globalBindings.size(),
                                                               .pBindings = globalBindings.data()};

        // Set 1: Per-object data
        const VkDescriptorSetLayoutBinding objectBinding = vkinit::descriptorset_layout_binding(
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

        std::array objectBindings = {objectBinding};
        const VkDescriptorSetLayoutCreateInfo objectSetInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                               .flags = 0,
                                                               .bindingCount = objectBindings.size(),
                                                               .pBindings = objectBindings.data()};

        // Set 2: Per-material data (textures)
        const VkDescriptorSetLayoutBinding samplerBinding = vkinit::descriptorset_layout_binding(
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

        VkDescriptorSetLayoutCreateInfo materialSetInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                           .flags = 0,
                                                           .bindingCount = 1,
                                                           .pBindings = &samplerBinding};

        vkCreateDescriptorSetLayout(device, &globalSetInfo, nullptr, &globalSetLayout);
        vkCreateDescriptorSetLayout(device, &objectSetInfo, nullptr, &objectSetLayout);
        vkCreateDescriptorSetLayout(device, &materialSetInfo, nullptr, &materialSetLayout);

        const size_t sceneParamBufferSize = MaxFramesInFlight * uniformBufferSizeAlignUp(sizeof(uniform::SceneData));
        sceneParametersBuffer = createUniformBuffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        for (auto &frame : frames) {
            // Allocate the object buffer
            constexpr int MAX_OBJECTS = 10000;
            frame.objectBuffer = createUniformBuffer(sizeof(uniform::ObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

            // Add to the deletion queue
            mainDeletionQueue.pushFunction([&] {
                vmaDestroyBuffer(allocator, frame.objectBuffer.buffer, frame.objectBuffer.allocation);
            });

            // Set 0: Allocate a global descriptor set per frame
            VkDescriptorSetAllocateInfo globalSetAlloc = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                          .descriptorPool = descriptorPool,
                                                          .descriptorSetCount = 1,
                                                          .pSetLayouts = &globalSetLayout};

            vkAllocateDescriptorSets(device, &globalSetAlloc, &frame.globalDescriptor);

            VkDescriptorBufferInfo sceneInfo = {.buffer = sceneParametersBuffer.buffer,
                                                .offset = 0,
                                                .range = sizeof(uniform::SceneData)};

            const auto sceneWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                                                    frame.globalDescriptor, &sceneInfo, 0);

            // Set 1: Allocate an object descriptor set per frame
            VkDescriptorSetAllocateInfo objectSetAlloc = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                          .descriptorPool = descriptorPool,
                                                          .descriptorSetCount = 1,
                                                          .pSetLayouts = &objectSetLayout};

            vkAllocateDescriptorSets(device, &objectSetAlloc, &frame.objectDescriptor);

            VkDescriptorBufferInfo objectInfo = {.buffer = frame.objectBuffer.buffer,
                                                 .offset = 0,
                                                 .range = sizeof(uniform::ObjectData) * MAX_OBJECTS};

            const auto objectWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                                     frame.objectDescriptor, &objectInfo, 0);

            // Update the descriptor sets
            std::array setWrites = {sceneWrite, objectWrite};
            vkUpdateDescriptorSets(device, setWrites.size(), setWrites.data(), 0, nullptr);
        }

        mainDeletionQueue.pushFunction([=, this] {
            vmaDestroyBuffer(allocator, sceneParametersBuffer.buffer, sceneParametersBuffer.allocation);

            vkDestroyDescriptorSetLayout(device, globalSetLayout, nullptr);
            vkDestroyDescriptorSetLayout(device, objectSetLayout, nullptr);
            vkDestroyDescriptorSetLayout(device, materialSetLayout, nullptr);
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        });
    }

    void VulkanEngine::initPipelines() {
        // Build the stage-create-info for both vertex and fragment stages
        PipelineBuilder pipelineBuilder;

        // Setup vertex input state
        pipelineBuilder.vertexInputInfo = vkinit::vertex_input_state_create_info();
        const VertexInputDescription vertexDescription = Vertex::getVertexDescription();

        // Connect the pipeline builder vertex input info to the one we get from Vertex
        pipelineBuilder.vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
        pipelineBuilder.vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
        pipelineBuilder.vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
        pipelineBuilder.vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

        // Input assembly is the configuration for drawing triangle lists, strips, or individual points
        // We are just going to draw triangle list
        pipelineBuilder.inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        // Configure the rasterizer to draw filled triangles
        pipelineBuilder.rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

        // We don't use multisampling, so just run the default one
        pipelineBuilder.multisampling = vkinit::multisampling_state_create_info();

        // A single blend attachment with no blending and writing to RGBA
        pipelineBuilder.colorBlendAttachment = vkinit::color_blend_attachment_state();

        // Default depth testing
        pipelineBuilder.depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

        VkPipelineLayoutCreateInfo meshPipelineLayoutInfo = vkinit::pipeline_layout_create_info();

        // Setup push constants
        const VkPushConstantRange pushConstant = {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                                                  .offset = 0,
                                                  .size = sizeof(uniform::MeshPushConstants)};

        meshPipelineLayoutInfo.pPushConstantRanges = &pushConstant;
        meshPipelineLayoutInfo.pushConstantRangeCount = 1;

        VkPipelineLayoutCreateInfo texturePipelineLayoutInfo = meshPipelineLayoutInfo;

        const std::array setLayouts = {globalSetLayout, objectSetLayout};
        meshPipelineLayoutInfo.pSetLayouts = setLayouts.data();
        meshPipelineLayoutInfo.setLayoutCount = setLayouts.size();

        const std::array textureSetLayouts = {globalSetLayout, objectSetLayout, materialSetLayout};
        texturePipelineLayoutInfo.pSetLayouts = textureSetLayouts.data();
        texturePipelineLayoutInfo.setLayoutCount = textureSetLayouts.size();

        VkPipelineLayout meshPipelineLayout;
        VK_CHECK(vkCreatePipelineLayout(device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout));
        VkPipelineLayout texturePipelineLayout;
        VK_CHECK(vkCreatePipelineLayout(device, &texturePipelineLayoutInfo, nullptr, &texturePipelineLayout));

        // Create the default mesh pipeline
        createMaterial(pipelineBuilder, meshPipelineLayout, "shaders/default.vert.spv", "shaders/default_lit.frag.spv", "defaultmesh");
        // Create the drone pipeline
        createMaterial(pipelineBuilder, meshPipelineLayout, "shaders/drone.vert.spv", "shaders/drone_lighting.frag.spv", "drone");
        // Create the simple map pipeline
        createMaterial(pipelineBuilder, meshPipelineLayout, "shaders/map.vert.spv", "shaders/map_simple.frag.spv", "map_simple");
        // Create the textured map pipeline
        createMaterial(pipelineBuilder, texturePipelineLayout, "shaders/map.vert.spv", "shaders/map_textured.frag.spv", "map_textured");
        // Create the triangle tester pipeline
        createMaterial(pipelineBuilder, meshPipelineLayout, "shaders/triangle_tester.vert.spv", "shaders/triangle_tester.frag.spv", "triangle_tester");

        mainDeletionQueue.pushFunction([=, this] {
            vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
            vkDestroyPipelineLayout(device, texturePipelineLayout, nullptr);
        });
    }

    void VulkanEngine::createMaterial(PipelineBuilder &pipelineBuilder, VkPipelineLayout pipelineLayout, const std::string &vertexShaderPath, const std::string &fragmentShaderPath, const std::string &materialName) {
        // Compile default mesh vertex shader
        auto meshVertShader = loadShaderModule(vertexShaderPath);
        if (!meshVertShader)
            std::cout << "Error when building the mesh vertex shader module: " << vertexShaderPath << std::endl;
        else
            std::cout << "Mesh vertex shader: " << vertexShaderPath << " successfully loaded" << std::endl;

        auto fragmentShader = loadShaderModule(fragmentShaderPath);
        if (!fragmentShader)
            std::cout << "Error when building the fragment shader: " << fragmentShaderPath << std::endl;
        else
            std::cout << "Fragment shader: " << fragmentShaderPath << " successfully loaded" << std::endl;

        pipelineBuilder.pipelineLayout = pipelineLayout;

        // Add the shader to the pipeline
        pipelineBuilder.shaderStages.push_back(
                vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, *meshVertShader));
        pipelineBuilder.shaderStages.push_back(
                vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, *fragmentShader));

        // Build the mesh triangle pipeline
        VkPipeline meshPipeline = pipelineBuilder.buildPipeline(device, renderPass);

        createMaterial(materialName, meshPipeline, pipelineLayout);

        // Delete all the vulkan shaders
        vkDestroyShaderModule(device, *meshVertShader, nullptr);
        vkDestroyShaderModule(device, *fragmentShader, nullptr);

        // Add the pipelines to the deletion queue
        mainDeletionQueue.pushFunction([=, this] {
            vkDestroyPipeline(device, meshPipeline, nullptr);
        });

        pipelineBuilder.shaderStages.clear();
    }

    void VulkanEngine::initCommands() {
        // Require the pool to allow for resetting of individual command buffers
        const VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(
                graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        // Create a command pool per frame, with one command buffer in it
        for (auto &frame : frames) {
            VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frame.commandPool));

            // Allocate the default command buffer that we will use for rendering
            VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frame.commandPool, 1);

            VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frame.mainCommandBuffer));

            // Enqueue the destruction of the pool
            mainDeletionQueue.pushFunction([=, this] {
                vkDestroyCommandPool(device, frame.commandPool, nullptr);
            });
        }

        // Create the command pool for the upload context
        const VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily);
        VK_CHECK(vkCreateCommandPool(device, &uploadCommandPoolInfo, nullptr, &uploadContext.commandPool));

        mainDeletionQueue.pushFunction([=, this] {
            vkDestroyCommandPool(device, uploadContext.commandPool, nullptr);
        });

        // Allocate the default command buffer that we will use for the instant commands
        const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(uploadContext.commandPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &uploadContext.commandBuffer));
    }

    void VulkanEngine::initSyncStructures() {
        // We want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
        const VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
        const VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

        for (auto &frame : frames) {
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &frame.renderFence));
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frame.presentSemaphore));
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frame.renderSemaphore));

            // Enqueue the destruction of the fence and semaphores
            mainDeletionQueue.pushFunction([=, this] {
                vkDestroyFence(device, frame.renderFence, nullptr);
                vkDestroySemaphore(device, frame.presentSemaphore, nullptr);
                vkDestroySemaphore(device, frame.renderSemaphore, nullptr);
            });
        }

        const VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();
        VK_CHECK(vkCreateFence(device, &uploadFenceCreateInfo, nullptr, &uploadContext.uploadFence));

        mainDeletionQueue.pushFunction([=, this] {
            vkDestroyFence(device, uploadContext.uploadFence, nullptr);
        });
    }
} // namespace dfv
