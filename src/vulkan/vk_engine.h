#pragma once

#include <array>
#include <filesystem>
#include <span>
#include <vector>

#include <vk_mem_alloc.h>

#include "deletion_queue.h"
#include "render_object.h"
#include "vk_mesh.h"
#include "vk_traits.h"
#include "vk_types.h"

namespace dfv {

    constexpr unsigned int MaxFramesInFlight = 2;

    struct CameraData {
        glm::mat4 view; // View matrix (camera location/transform)
        glm::mat4 proj; // Projection matrix (perspective)
        glm::mat4 viewproj; // View * Projection matrix to avoid multiplication in the shader
    };

    struct SceneData {
        glm::vec4 fogColor; // w is for exponent
        glm::vec4 fogDistances; //x for min, y for max, zw unused.
        glm::vec4 ambientColor;
        glm::vec4 sunlightDirection; //w for sun power
        glm::vec4 sunlightColor;
    };

    struct ObjectData {
        glm::mat4 modelMatrix;
    };

    /**
     * A structure containing per-frame data used by the engine.
     */
    struct FrameData {
        VkSemaphore presentSemaphore, renderSemaphore;
        VkFence renderFence;

        VkCommandPool commandPool;
        VkCommandBuffer mainCommandBuffer;

        AllocatedBuffer cameraBuffer; //!< Buffer containing a single CameraData object to use for the frame.

        VkDescriptorSet globalDescriptor; //!< Global descriptor set for the frame.

        AllocatedBuffer objectBuffer;
        VkDescriptorSet objectDescriptor;
    };

    class VulkanEngine {
      public:
        bool isInitialized{false};
        int frameNumber{0};

        VulkanTraits traits; //!< Vulkan traits of the current device

        VmaAllocator allocator; //!< Vulkan Memory Allocator library handle
        DeletionQueue mainDeletionQueue; //!< A queue containing Vulkan objects to be deleted when the engine is shut down

        VkExtent2D windowExtent; //!< The size of the window

        VkInstance instance; //!< Vulkan library handle
        VkDebugUtilsMessengerEXT debugMessenger; //!< Vulkan debug output handle
        VkPhysicalDevice chosenGPU; //!< GPU chosen as the default device
        VkDevice device; //!< Vulkan device for commands
        VkSurfaceKHR surface; //!< Vulkan window surface

        VkSwapchainKHR swapchain; // Swapchain to present images to the screen
        VkFormat swapchainImageFormat; // Image format expected by the windowing system
        std::vector<VkImage> swapchainImages; // Array of images from the swapchain
        std::vector<VkImageView> swapchainImageViews; // Array of image-views from the swapchain

        VkQueue graphicsQueue; //!< The queue command buffers will be submitted to
        uint32_t graphicsQueueFamily; //!< The family of the queue

        VkRenderPass renderPass; //!< Default renderpass for the engine
        std::vector<VkFramebuffer> framebuffers;

        std::array<FrameData, MaxFramesInFlight> frames; //!< Per-frame data

        VkImageView depthImageView;
        AllocatedImage depthImage;
        VkFormat depthFormat;

        VkDescriptorSetLayout globalSetLayout; //!< The layout for the global descriptor set
        VkDescriptorSetLayout objectSetLayout; //!< The layout for the object descriptor set
        VkDescriptorPool descriptorPool; //!< Global descriptor pool

        std::vector<RenderObject> renderObjects; //!< The objects to render

        std::unordered_map<std::string, Mesh> meshes; //!< Meshes loaded by the engine
        std::unordered_map<std::string, Material> materials; //!< Materials loaded by the engine

        SceneData sceneParameters; //!< Scene parameters to use during rendering
        AllocatedBuffer sceneParametersBuffer; //!< Buffer containing the scene parameters

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
         * Draws and displays the next frame.
         */
        void draw();


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
         * Creates a new mesh with the given name.
         * @param name The name of the mesh, used to identify it later.
         * @param filename The path to the mesh file to load.
         * @return A pointer to the created mesh.
         */
        Mesh *createMesh(const std::string &name, const std::filesystem::path &filename);

        /**
         * Gets the mesh with the given name.
         * @param name The name of the mesh to get.
         * @return The mesh with the given name, or nullptr if no mesh with that name exists.
         */
        Mesh *getMesh(const std::string &name);

      private:
        /**
         * Draws all objects using the given command buffer.
         * @param cmdBuf The command buffer to use for drawing.
         */
        void drawObjects(VkCommandBuffer cmdBuf);

        /**
         * Gets the frame data to use for the current frame.
         */
        FrameData &getCurrentFrame();

        /**
         * Load a shader module from the given file path.
         * @param filePath The path to the shader module.
         * @param outShaderModule A pointer to the shader module to write to.
         * @return True if the shader module was loaded successfully, false otherwise.
         */
        bool loadShaderModule(const std::filesystem::path &filePath, VkShaderModule *outShaderModule) const;

        /**
         * Creates a new buffer with the given parameters.
         * @param allocSize The size of the buffer to allocate.
         * @param usage The usage flags for the buffer.
         * @param memoryUsage The memory usage flags for the buffer.
         * @return The created buffer.
         */
        AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const;

        /**
         * Uploads the given mesh to the GPU.
         * @param mesh The mesh to upload.
         */
        void uploadMesh(Mesh &mesh);

        /**
         * Aligns the given size to the minimum uniform buffer offset alignment requirements.
         */
        size_t uniformBufferSizeAlignUp(size_t size) const;

        /**
         * Loads the core Vulkan structures
         * @param window The GLFW window to retrieve the surface from.
         */
        void initVulkan(GLFWwindow *window);
        void initSwapchain();
        void initCommands();
        void initSyncStructures();
        void initDefaultRenderpass();
        void initFramebuffers();
        void initDescriptors();
        void initPipelines();

        void loadMeshes();
        void initScene();
    };

} // namespace dfv
