#pragma once

#include <vector>
#include <optional>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace dfv::vulkan {
    class VulkanContext {
    public:
        explicit VulkanContext();

        ~VulkanContext();

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
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffer;

        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;

        // Disallow copy and assignment
        VulkanContext(const VulkanContext &) = delete;

        VulkanContext &operator=(const VulkanContext &) = delete;

        VulkanContext(VulkanContext &&) = delete;

        VulkanContext &operator=(VulkanContext &&) = delete;

        void drawFrame();

    private:
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

        void createGraphicsPipeline();

        void createFrameBuffers();

        void createCommandPool();

        void createCommandBuffer();

        void createSyncObjects();

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

        /**
         * create a shader module from a giver shader bytecode
         * @param code
         * @return
         */
        VkShaderModule createShaderModule(const std::vector<char>& code) const;
    };
}
