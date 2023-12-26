#pragma once

#include <array>
#include <filesystem>
#include <vector>

#include <vk_mem_alloc.h>

#include "deletion_queue.h"
#include "render_object.h"
#include "surface_wrapper.h"
#include "uniform_types.h"
#include "vk_mesh.h"
#include "vk_pipeline.h"
#include "vk_texture.h"
#include "vk_traits.h"
#include "vk_types.h"

namespace dfv {

    constexpr unsigned int MaxFramesInFlight = 2;

    /**
     * A structure containing per-frame data used by the engine.
     */
    struct FrameData {
        VkSemaphore presentSemaphore{VK_NULL_HANDLE}; //!< Semaphore that is signaled when the frame has been presented to the screen
        VkSemaphore renderSemaphore{VK_NULL_HANDLE}; //!< Semaphore that is signaled when rendering has finished
        VkFence renderFence{VK_NULL_HANDLE}; //!< Fence that is signaled when the frame has finished rendering

        VkCommandPool commandPool{VK_NULL_HANDLE};
        VkCommandBuffer mainCommandBuffer{VK_NULL_HANDLE};

        VkDescriptorSet globalDescriptor{VK_NULL_HANDLE}; //!< Global descriptor set for the frame.

        AllocatedBuffer objectBuffer;
        VkDescriptorSet objectDescriptor{VK_NULL_HANDLE};
    };

    /**
     * A structure containing data used for GPU upload commands.
     */
    struct UploadContext {
        VkFence uploadFence{VK_NULL_HANDLE};
        VkCommandPool commandPool{VK_NULL_HANDLE};
        VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    };

    class VulkanEngine {
      public:
        explicit VulkanEngine(const SurfaceWrapper &surfaceWrapper)
            : surfaceWrap(surfaceWrapper) {}

        /**
         * Initializes the engine.
         * @note This function will throw exceptions if initialization fails.
         */
        void init();

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
         * Creates a new mesh with the given name from an model file.
         * @param name The name of the mesh, used to identify it later.
         * @param filename The path to the mesh file to load.
         * @return A pointer to the created mesh.
         */
        Mesh *createMesh(const std::string &name, const std::filesystem::path &filename);

        /**
         * Inserts an already loaded mesh into the engine.
         * @param name The name of the mesh, used to identify it later.
         * @param mesh The mesh to insert.
         * @return A pointer to the inserted mesh.
         */
        Mesh *insertMesh(const std::string &name, Mesh &&mesh);

        /**
         * Gets the mesh with the given name.
         * @param name The name of the mesh to get.
         * @return The mesh with the given name, or nullptr if no mesh with that name exists.
         */
        Mesh *getMesh(const std::string &name);

        /**
         * Creates a new texture with the given name from an image file.
         * @param name The name of the texture, used to identify it later.
         * @param filename The path to the image file to load.
         * @return A pointer to the created texture.
         */
        Texture *createTexture(const std::string &name, const std::filesystem::path &filename);

        /**
         * Inserts a texture from existing data into the engine.
         * Currently only the R8G8B8A8 format is supported.
         * @param name The name of the texture, used to identify it later.
         * @param texture A texture object the extent and format set.
         * @param data A span of the pixel data to use for the texture.
         * @return A pointer to the inserted texture.
         */
        Texture *insertTexture(const std::string &name, Texture &&texture, std::span<std::byte> data);

        /**
         * Gets the texture with the given name.
         * @param name The name of the texture to get.
         * @return The texture with the given name, or nullptr if no texture with that name exists.
         */
        Texture *getTexture(const std::string &name);

        /**
         * Allocates a new render object in the engine.
         * @return A structure holding a pointer to the allocated render object and its handle for later retrieval.
         */
        RenderObjectDescriptor allocateRenderObject();

        /**
         * Gets the render object with the given handle.
         * @param handle The handle of the render object to get.
         * @return The render object, or nullptr if no render object with that handle exists.
         */
        RenderObject *getRenderObject(RenderHandle handle);

        /**
         * @brief Returns the current frame number.
         */
        uint32_t getFrameNumber() const {
            return frameNumber;
        }

        struct {
            glm::vec3 position{}; //!< The position of the camera
            glm::vec3 orientation{}; //!< The orientation of the camera in radians (yaw, pitch, roll)
            glm::vec3 front{}; //!< Normalized vector pointing in the direction the camera is facing
            glm::vec3 up{}; //!< The up vector

            float fov{glm::radians(70.f)}; //!< Field of view in radians
            float nearPlane{0.1f}; //!< Near plane distance
            float farPlane{2000000.f}; //!< Far plane distance

            /**
             * @brief Updates the front vector based on the current orientation.
             */
            void updateFront() {
                const glm::vec3 direction = {std::cos(orientation.x) * std::cos(orientation.y),
                                             std::sin(orientation.y),
                                             std::sin(orientation.x) * std::cos(orientation.y)};

                front = glm::normalize(direction);
            }

            /**
             * @brief Updates the orientation based on the current front vector.
             */
            void updateOrientation() {
                orientation.x = std::atan2(front.z, front.x);
                orientation.y = std::asin(front.y);
            }
        } camera; //!< Camera parameters to use during rendering

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
         * Immediately submits a command buffer to the graphics queue and waits for completion.
         * @param submitFunc The function to call to fill the command buffer.
         */
        void immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&submitFunc) const;

        /**
         * Load a shader module from the given file path.
         * @param filePath The path to the shader module.
         * @return The loaded shader module, or std::nullopt if loading failed.
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
         * Creates a new staging buffer with the given parameters.
         * @param allocSize The size of the buffer to allocate.
         * @return The created staging buffer.
         */
        AllocatedBuffer createStagingBuffer(size_t allocSize) const;

        /**
         * Uploads the given mesh to the GPU.
         * @param mesh The mesh to upload.
         */
        void uploadMesh(Mesh &mesh);

        /**
         * Uploads the given texture to the GPU.
         * @param texture The texture to upload.
         * @param data The data to upload.
         */
        void uploadTexture(Texture &texture, std::span<std::byte> data);

        /**
         * Aligns the given size to the minimum uniform buffer offset alignment requirements.
         */
        size_t uniformBufferSizeAlignUp(size_t size) const;

        void initVulkan();

        void initSwapchain();
        void initDefaultRenderpass();
        void initFramebuffers();

        void initDescriptors();
        void initPipelines();

        void initCommands();
        void initSyncStructures();

        void recreateSwapchain();

      private:
        const SurfaceWrapper &surfaceWrap; //!< The surface wrapper used to retrieve the surface

        bool isInitialized{false};
        uint32_t frameNumber{0};

        VulkanTraits traits; //!< Vulkan traits of the current device

        VmaAllocator allocator{VK_NULL_HANDLE}; //!< Vulkan Memory Allocator library handle
        DeletionQueue mainDeletionQueue; //!< A queue containing Vulkan objects to be deleted when the engine is shut down
        DeletionQueue swapchainDeletionQueue; //!< A queue for swapchain-related Vulkan objects to be deleted when the swapchain is recreated

        VkExtent2D windowExtent{}; //!< The size of the window

        VkInstance instance{VK_NULL_HANDLE}; //!< Vulkan library handle
        VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE}; //!< Vulkan debug output handle
        VkPhysicalDevice chosenGPU{VK_NULL_HANDLE}; //!< GPU chosen as the default device
        VkDevice device{VK_NULL_HANDLE}; //!< Vulkan device for commands
        VkSurfaceKHR surface{VK_NULL_HANDLE}; //!< Vulkan window surface

        VkSwapchainKHR swapchain{VK_NULL_HANDLE}; // Swapchain to present images to the screen
        VkFormat swapchainImageFormat{VK_FORMAT_UNDEFINED}; // Image format expected by the windowing system
        std::vector<VkImage> swapchainImages; // Array of images from the swapchain
        std::vector<VkImageView> swapchainImageViews; // Array of image-views from the swapchain

        VkQueue graphicsQueue{VK_NULL_HANDLE}; //!< The queue command buffers will be submitted to
        uint32_t graphicsQueueFamily{}; //!< The family of the queue

        VkRenderPass renderPass{VK_NULL_HANDLE}; //!< Default renderpass for the engine
        VkFormat renderPassColorFormat{VK_FORMAT_UNDEFINED}; //!< Format used by the renderpass color attachment
        std::vector<VkFramebuffer> framebuffers;

        std::array<FrameData, MaxFramesInFlight> frames; //!< Per-frame data

        UploadContext uploadContext; //!< Data for GPU upload commands

        VkImageView depthImageView{VK_NULL_HANDLE};
        AllocatedImage depthImage;
        VkFormat depthFormat{VK_FORMAT_UNDEFINED};

        VkDescriptorSetLayout globalSetLayout{VK_NULL_HANDLE}; //!< The layout for the global descriptor set
        VkDescriptorSetLayout objectSetLayout{VK_NULL_HANDLE}; //!< The layout for the object descriptor set
        VkDescriptorSetLayout materialSetLayout{VK_NULL_HANDLE}; //!< The layout for the material descriptor set, containing one sampler for a single texture
        VkDescriptorPool descriptorPool{VK_NULL_HANDLE}; //!< Global descriptor pool

        std::vector<RenderObject> renderObjects; //!< The objects to render

        std::unordered_map<std::string, Mesh> meshes; //!< Meshes loaded by the engine
        std::unordered_map<std::string, Material> materials; //!< Materials loaded by the engine
        std::unordered_map<std::string, Texture> textures; //!< Textures loaded by the engine

        uniform::SceneData sceneParameters{}; //!< Scene parameters to use during rendering
        AllocatedBuffer sceneParametersBuffer; //!< Buffer containing the scene parameters
        void createMaterial(PipelineBuilder &pipelineBuilder, VkPipelineLayout meshPipelineLayout, const std::string &vertexShaderPath, const std::string &fragmentShaderPath, const std::string &materialName);
    };

} // namespace dfv
