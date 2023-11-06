#pragma once

#include "vulkan/surface_wrapper.h"

struct GLFWwindow;

namespace dfv {
    class GlfwSurface : public SurfaceWrapper {
      public:
        explicit GlfwSurface(GLFWwindow *window);

        std::pair<VkResult, VkSurfaceKHR> getSurface(VkInstance instance) override;

        VkExtent2D getExtent() override;

      private:
        GLFWwindow *window;
    };
} // namespace dfv
