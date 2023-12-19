#pragma once

#include "vk_types.h"

#include <utility>

namespace dfv {
    /**
     * @brief A generic wrapper for a surface, used to retrieve a VkSurfaceKHR for use with the Vulkan engine.
     */
    class SurfaceWrapper {
      public:
        virtual ~SurfaceWrapper() = default;
        /**
         * @brief Retrieves a surface for use with the given Vulkan instance.
         * @param instance The Vulkan instance.
         * @return A pair containing the result of the operation and the surface if successful.
         */
        virtual std::pair<VkResult, VkSurfaceKHR> getSurface(VkInstance instance) const = 0;

        /**
         * @brief Retrieves the extent of the surface.
         */
        virtual VkExtent2D getExtent() const = 0;
    };
} // namespace dfv
