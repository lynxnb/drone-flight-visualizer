#pragma once

#include "vk_types.h"

#include <utility>

namespace dfv {
    /**
     * @brief A generic wrapper for a surface, used to retrieve a VkSurfaceKHR for use with the Vulkan engine.
     */
    class SurfaceWrapper {
      public:
        virtual std::pair<VkResult, VkSurfaceKHR> getSurface(VkInstance instance) = 0;
    };
} // namespace dfv
