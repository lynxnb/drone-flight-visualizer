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

        // Disallow copy and assignment
        VulkanContext(const VulkanContext &) = delete;

        VulkanContext &operator=(const VulkanContext &) = delete;

        VulkanContext(VulkanContext &&) = delete;

        VulkanContext &operator=(VulkanContext &&) = delete;

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

        void CreateWindow();

        void CreateInstance();

        void CreateSurface();

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice pDevice) const;

        bool IsDeviceSuitable(VkPhysicalDevice pDevice);

        void SelectPhysicalDevice();

        void CreateLogicalDevice();

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);

        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

        void CreateSwapChain();

        void CreateImageViews();
    };
}
