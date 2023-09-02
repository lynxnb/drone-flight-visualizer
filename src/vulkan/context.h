#pragma once

#include <vector>
#include <array>
#include <glm.hpp>
#include <optional>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace dfv::vulkan {
    class VulkanContext {
    public:
        explicit VulkanContext();

        ~VulkanContext();

        size_t currentFrame = 0;
        GLFWwindow *window;
        VkInstance instance;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device;
        VkQueue graphicsQueue;


        VkSurfaceKHR surface;
        VkQueue presentQueue;
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;

        std::vector<VkImageView> swapChainImageViews;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkRenderPass renderPass;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;

        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void *> uniformBuffersMapped;

        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;


        struct Vertex {
            glm::vec2 pos;
            glm::vec3 color;

            static VkVertexInputBindingDescription getBindingDescription() {
                VkVertexInputBindingDescription bindingDescription{};
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(Vertex);
                // can be per vertex or per instance
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


                return bindingDescription;
            }

            static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
                std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

                attributeDescriptions[0].binding = 0;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(Vertex, pos);

                attributeDescriptions[1].binding = 0;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(Vertex, color);
                return attributeDescriptions;
            }
        };

        const std::vector<Vertex> vertices = {
                {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                {{0.5f,  -0.5f}, {0.0f, 1.0f, 0.0f}},
                {{0.5f,  0.5f},  {0.0f, 0.0f, 1.0f}},
                {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}}
        };

        const std::vector<uint16_t> indices = {
                0, 1, 2, 2, 3, 0
        };

        struct UniformBufferObject {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        };


        // Disallow copy and assignment
        VulkanContext(const VulkanContext &) = delete;

        VulkanContext &operator=(const VulkanContext &) = delete;

        VulkanContext(VulkanContext &&) = delete;

        VulkanContext &operator=(VulkanContext &&) = delete;

        void drawFrame();

    private:
        const size_t MAX_FRAMES_IN_FLIGHT = 2;
        struct QueueFamilyIndices {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            bool IsComplete() const {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        void createWindow();

        void createInstance();

        void createSurface();

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice pDevice) const;

        bool isDeviceSuitable(VkPhysicalDevice pDevice);

        void selectPhysicalDevice();

        void createLogicalDevice();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

        void createSwapChain();

        void createImageViews();

        void createRenderPass();

        void createDescriptorSetLayout();

        void createGraphicsPipeline();

        void createFrameBuffers();

        void createCommandPool();

        void createCommandBuffer();

        void createSyncObjects();

        void createVertexBuffer();

        void createIndexBuffer();

        void createUniformBuffers();

        void createDescriptorPool();

        void createDescriptorSets();

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        void
        createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                     VkDeviceMemory &bufferMemory);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        void updateUniformBuffer(uint32_t currentImage);

        /**
         * create a shader module from a giver shader bytecode
         * @param code
         * @return
         */
        VkShaderModule createShaderModule(const std::vector<char> &code) const;
    };
}
