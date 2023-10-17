#pragma once

#include <array>
#include <filesystem>
#include <span>
#include <vector>

#include <vk_mem_alloc.h>

#include "deletion_queue.h"
#include "render_object.h"
#include "vk_mesh.h"
#include "vk_types.h"
#include "vk_traits.h"

namespace dfv {

    constexpr unsigned int MaxFramesInFlight = 2;

    struct CameraData {
        glm::mat4 view; // View matrix (camera location/transform)
        glm::mat4 proj; // Projection matrix (perspective)
        glm::mat4 viewproj; // View * Projection matrix to avoid multiplication in the shader
    };

    struct FrameData {
        VkSemaphore presentSemaphore, renderSemaphore;
        VkFence renderFence;

        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;

        AllocatedBuffer cameraBuffer; //!< Buffer containing a single CameraData object to use for the frame.

        VkDescriptorSet globalDescriptor; //!< Global descriptor set for the frame.
    };

    class VulkanEngine {
      public:
        bool isInitialized{false};
        int frameNumber{0};

        VulkanTraits traits;

        VmaAllocator allocator;
        DeletionQueue mainDeletionQueue;

        VkExtent2D windowExtent;

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

        VkRenderPass renderPass;
        std::vector<VkFramebuffer> framebuffers;

        std::array<FrameData, MaxFramesInFlight> frames;

        VkImageView depthImageView;
        AllocatedImage depthImage;
        VkFormat depthFormat;

        VkDescriptorSetLayout globalSetLayout;
        VkDescriptorPool descriptorPool;

        std::vector<RenderObject> renderables;

        std::unordered_map<std::string, Mesh> meshes;
        std::unordered_map<std::string, Material> materials;

        int selectedShader{0};

        /**
         * Initializes the engine.
         * @param window The GLFWwindow object to retrieve the surface from.
         * @param width The width of the window.
         * @param height The height of the window.
         * @note This function will throw exceptions if initialization fails.
         */
        void init(GLFWwindow *window, uint32_t width, uint32_t height);

        /**
         * Shuts down the engine
         */
        void cleanup();

        /**
         * Draws loop
         */
        void draw();

        bool loadShaderModule(const std::filesystem::path &filePath, VkShaderModule *outShaderModule) const;

        AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;

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
         * @param window The GLFW window to retrieve the surface from.
         */
        void initVulkan(GLFWwindow *window);

        /**
         * Creates the swapchain
         */
        void initSwapchain();

        /**
         * Creates the command pool and the default command buffer
         */
        void initCommands();

        /**
         * Creates the synchronization structures
         */
        void initSyncStructures();

        /**
         * Creates the default renderpass
         */
        void initDefaultRenderpass();

        /**
         * Creates the framebuffers for the swapchain images
         */
        void initFramebuffers();

        void initDescriptors();

        void initPipelines();

        void loadMeshes();

        void uploadMesh(Mesh &mesh);

        void initScene();

        FrameData &getCurrentFrame();
    };

} // namespace dfv
