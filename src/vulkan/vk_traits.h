#pragma once

#include <cstdint>
#include <format>
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
        size_t maxPushConstantsSize{};

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
            maxPushConstantsSize = props.limits.maxPushConstantsSize;
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
                               "* Min uniform buffer offset alignment: {}\n"
                               "* Max push constants size: {}",
                               deviceName,
                               apiVersion,
                               driverVersion,
                               minUniformBufferOffsetAlignment,
                               maxPushConstantsSize);
        }
    };

} // namespace dfv
