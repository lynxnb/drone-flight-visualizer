#pragma once

#include <array>
#include <filesystem>
#include <span>
#include <vector>

#include "deletion_queue.h"
#include "render_object.h"
#include "vk_mesh.h"
#include "vk_traits.h"
#include "vk_types.h"
#include "surface_wrapper.h"

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

    /**
     * A structure containing data used for GPU upload commands.
     */
    struct UploadContext {
        VkFence uploadFence;
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;
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

        UploadContext uploadContext; //!< Data for GPU upload commands

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

        struct {
            glm::vec3 position; //!< The position of the camera
            glm::vec3 orientation; //!< The orientation of the camera (x = pitch, y = yaw, z = roll)

            float fov; //!< Field of view in radians
            float nearPlane; //!< Near plane distance
            float farPlane; //!< Far plane distance

            float movementSpeed; //!< Movement speed in units per second
            std::atomic<float> speedMultiplier = 1.0f; //!< Speed multiplier (used for sprinting)
            float rotationSpeed; //!< Rotation speed in radians per second

            std::atomic<float> surgeDirection = 0; //!< Whether the camera is moving forward or backwards (1 for forward, -1 for backwards)
            std::atomic<float> swayDirection = 0; //!< Whether the camera is moving left or right (1 for right, -1 for left)
            std::atomic<float> heaveDirection = 0; //!< Whether the camera is moving up or down (-1 for up, 1 for down)

            std::atomic<float> yawDirection = 0; //!< Whether the camera is rotating left or right (1 for right, -1 for left)
            std::atomic<float> pitchDirection = 0; //!< Whether the camera is rotating up or down (1 for up, -1 for down)

            /**
             * @brief Gets the adjusted movement speed, taking into account the speed multiplier.
             * @return The adjusted movement speed.
             */
            float adjustedMovementSpeed() {
                return movementSpeed * speedMultiplier.load(std::memory_order_relaxed);
            }
        } cameraParameters; //!< Camera parameters to use during rendering

        /**
         * Initializes the engine.
         * @param surfaceWrap The surface wrapper to use for retrieve the surface.
         * @param width The width of the window.
         * @param height The height of the window.
         * @note This function will throw exceptions if initialization fails.
         */
        void init(SurfaceWrapper &surfaceWrap, uint32_t width, uint32_t height);

        /**
         * Shuts down the engine
         */
        void cleanup();

        /**
         * Draws and displays the next frame.
         */
        void draw();

        /**
         * Updates render objects by calling their update functions.
         * @param deltaTime The time since the last frame in seconds.
         */
        void update(seconds_f deltaTime);

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
         * Updates the camera data for the current frame.
         * @param deltaTime The time since the last frame in seconds.
         */
        void updateCamera(seconds_f deltaTime);

        /**
         * Gets the frame data to use for the current frame.
         */
        FrameData &getCurrentFrame();

        /**
         * Immediately submits a command buffer to the graphics queue and waits for completion.
         * @param submitFunc The function to call to fill the command buffer.
         */
        void immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&submitFunc);

        /**
         * Load a shader module from the given file path.
         * @param filePath The path to the shader module.
         * @param outShaderModule A pointer to the shader module to write to.
         * @return True if the shader module was loaded successfully, false otherwise.
         */
        std::optional<VkShaderModule> loadShaderModule(const std::filesystem::path &filePath) const;

        /**
         * Creates a new uniform buffer with the given parameters.
         * @param allocSize The size of the buffer to allocate.
         * @param usage The usage flags for the buffer.
         * @return The created uniform buffer.
         */
        AllocatedBuffer createUniformBuffer(size_t allocSize, VkBufferUsageFlags usage) const;

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
         * @param surfaceWrap The surface wrapper to use for retrieve the surface.
         */
        void initVulkan(SurfaceWrapper &surfaceWrap);
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
