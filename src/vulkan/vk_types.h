#pragma once

#include <vulkan/vulkan.h>

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
