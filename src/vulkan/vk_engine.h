﻿#pragma once

#include <vector>

#include "vk_types.h"

namespace dfv {

    class VulkanEngine {
    public:
        bool isInitialized{false};
        int frameNumber{0};

        VkExtent2D windowExtent{1700, 900};
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
    };

}
