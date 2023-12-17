#pragma once

#include <vulkan/vulkan.h>

#define VMA_VULKAN_VERSION 1001000
#include <vk_mem_alloc.h>

namespace dfv {

    struct AllocatedBuffer {
        VkBuffer buffer{VK_NULL_HANDLE};
        VmaAllocation allocation{VK_NULL_HANDLE};
    };

    struct AllocatedImage {
        VkImage image{VK_NULL_HANDLE};
        VmaAllocation allocation{VK_NULL_HANDLE};
    };

} // namespace dfv
