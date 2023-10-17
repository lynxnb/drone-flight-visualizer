#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace dfv {

    /**
     * A class that holds the Vulkan traits of the current device.
     */
    struct VulkanTraits {
        std::string deviceName;
        std::string apiVersion;
        std::string driverVersion;
        size_t minUniformBufferOffsetAlignment{};

        VulkanTraits() = default;

        explicit VulkanTraits(const VkPhysicalDeviceProperties &props) {
            deviceName = props.deviceName;
            apiVersion = std::format("{}.{}.{}",
                                     VK_VERSION_MAJOR(props.apiVersion),
                                     VK_VERSION_MINOR(props.apiVersion),
                                     VK_VERSION_PATCH(props.apiVersion));
            driverVersion = std::format("{}.{}.{}",
                                        VK_VERSION_MAJOR(props.driverVersion),
                                        VK_VERSION_MINOR(props.driverVersion),
                                        VK_VERSION_PATCH(props.driverVersion));

            minUniformBufferOffsetAlignment = props.limits.minUniformBufferOffsetAlignment;
        }

        /**
         * Returns a formatted human-readable summary of the Vulkan traits.
         */
        std::string summary() const {
            return std::format("Device:\n"
                               " Name: {}\n"
                               " Vulkan version: {}\n"
                               " Driver version: {}\n"
                               "Traits:\n"
                               "* Min uniform buffer offset alignment: {}",
                               deviceName,
                               apiVersion,
                               driverVersion,
                               minUniformBufferOffsetAlignment);
        }
    };

} // namespace dfv
