#pragma once

#include <filesystem>
#include <vector>
#include <span>

#include <vk_mem_alloc.h>

#include "deletion_queue.h"
#include "mesh.h"
#include "render_object.h"
#include "vk_types.h"

namespace dfv {

    class VulkanEngine {
      public:
        bool isInitialized{false};
        int frameNumber{0};

        VmaAllocator allocator;
        DeletionQueue mainDeletionQueue;

        VkExtent2D windowExtent{1280, 720};
        GLFWwindow *window{nullptr};

        VkInstance instance; // Vulkan library handle
        VkDebugUtilsMessengerEXT debugMessenger; // Vulkan debug output handle
        VkPhysicalDevice chosenGPU; // GPU chosen as the default device
        VkDevice device; // Vulkan device for commands
        VkSurfaceKHR surface; // Vulkan window surface

        VkSwapchainKHR swapchain; // Swapchain to present images to the screen
        VkFormat swapchainImageFormat; // Image format expected by the windowing system
        std::vector<VkImage> swapchainImages; // Array of images from the swapchain
        std::vector<VkImageView> swapchainImageViews; // Array of image-views from the swapchain

        VkQueue graphicsQueue; // Queue we will submit to
        uint32_t graphicsQueueFamily; // Family of that queue
        VkCommandPool commandPool; // The command pool for our commands
        VkCommandBuffer mainCommandBuffer; // The buffer we will record into

        VkRenderPass renderPass;
        std::vector<VkFramebuffer> framebuffers;

        VkSemaphore presentSemaphore, renderSemaphore;
        VkFence renderFence;

        VkPipelineLayout trianglePipelineLayout;
        VkPipelineLayout meshPipelineLayout;
        VkPipeline trianglePipeline;
        VkPipeline redTrianglePipeline;

        VkPipeline meshPipeline;
        Mesh triangleMesh;
        Mesh monkeyMesh;

        VkImageView depthImageView;
        AllocatedImage depthImage;
        VkFormat depthFormat;

        std::vector<RenderObject> renderables;

        std::unordered_map<std::string, Mesh> meshes;
        std::unordered_map<std::string, Material> materials;

        int selectedShader{0};

        /**
         * Initializes the engine
         */
        void init();

        /**
         * Shuts down the engine
         */
        void cleanup();

        /**
         * Draws loop
         */
        void draw();

        /**
         * Runs the main loop
         */
        void run();

        bool loadShaderModule(const std::filesystem::path &filePath, VkShaderModule *outShaderModule) const;

        /**
         * Creates a new render material with the given name.
         * @param name The name of the material, used to identify it later.
         * @param pipeline The pipeline to use for this material.
         * @param layout The pipeline layout to use for this material.
         * @return A pointer to the created material.
         */
        Material *createMaterial(const std::string &name, VkPipeline pipeline, VkPipelineLayout layout);

        /**
         * Gets the material with the given name.
         * @param name The name of the material to get.
         * @return The material with the given name, or nullptr if no material with that name exists.
         */
        Material *getMaterial(const std::string &name);

        /**
         * Gets the mesh with the given name.
         * @param name The name of the mesh to get.
         * @return The mesh with the given name, or nullptr if no mesh with that name exists.
         */
        Mesh *getMesh(const std::string &name);

        /**
         * Draws the given objects using the given command buffer.
         * @param cmdBuf The command buffer to use for drawing.
         * @param objects
         */
        void drawObjects(VkCommandBuffer cmdBuf, std::span<RenderObject> objects);

      private:
        /**
         * Loads the core Vulkan structures
         */
        void initVulkan();

        /**
         * Creates the swapchain
         */
        void initSwapchain();

        /**
         * Creates the command pool and the default command buffer
         */
        void initCommands();

        /**
         * Creates the default renderpass
         */
        void initDefaultRenderpass();

        /**
         * Creates the framebuffers for the swapchain images
         */
        void initFramebuffers();

        /**
         * Creates the synchronization structures
         */
        void initSyncStructures();

        void initPipelines();

        void loadMeshes();

        void uploadMesh(Mesh &mesh);

        void initScene();
    };

} // namespace dfv
