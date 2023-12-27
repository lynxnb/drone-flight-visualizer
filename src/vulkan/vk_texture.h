#pragma once

#include "vk_types.h"

namespace dfv {
    struct Texture {
        VkExtent3D extent{};
        VkFormat format{VK_FORMAT_UNDEFINED};
        AllocatedImage image;
        VkImageView imageView{VK_NULL_HANDLE};
    };
} // namespace dfv
