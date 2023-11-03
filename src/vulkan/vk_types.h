#pragma once

// clang-format off
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
// clang-format on

#define VMA_VULKAN_VERSION 1001000
#include <vk_mem_alloc.h>

namespace dfv {

    struct AllocatedBuffer {
        VkBuffer buffer;
        VmaAllocation allocation;
    };

    struct AllocatedImage {
        VkImage image;
        VmaAllocation allocation;
    };

} // namespace dfv
