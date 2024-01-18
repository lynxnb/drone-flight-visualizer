#pragma once

#include "vulkan/surface_wrapper.h"

struct GLFWwindow;

namespace dfv {
    class GlfwSurface : public SurfaceWrapper {
      public:
        explicit GlfwSurface(GLFWwindow *window);

        std::pair<VkResult, VkSurfaceKHR> getSurface(VkInstance instance) const override;

        VkExtent2D getExtent() const override;

        void initImgui() const override;

        void onFrameImgui() const override;

        void destroyImgui() const override;

      private:
        GLFWwindow *window{nullptr};
    };
} // namespace dfv
