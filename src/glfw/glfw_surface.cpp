#include "glfw_surface.h"

// clang-format off
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
// clang-format on

namespace dfv {
    GlfwSurface::GlfwSurface(GLFWwindow *window)
        : window(window) {}

    std::pair<VkResult, VkSurfaceKHR> GlfwSurface::getSurface(VkInstance instance) const {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        return {result, surface};
    }

    VkExtent2D GlfwSurface::getExtent() const {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        return {.width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height)};
    }
} // namespace dfv
