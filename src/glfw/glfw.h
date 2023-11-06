#pragma once

#include <GLFW/glfw3.h>

namespace dfv::raii {
    /**
     * @brief RAII wrapper for GLFW
     */
    class Glfw {
      public:
        Glfw(int width, int height, const char *title) {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Remove once swapchain recreation is implemented

            using namespace dfv::window_config;
            glfwWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
        }

        ~Glfw() {
            glfwDestroyWindow(glfwWindow);
            glfwTerminate();
        }

        GLFWwindow *getWindow() {
            return glfwWindow;
        }

      private:
        GLFWwindow *glfwWindow = nullptr;
    };
} // namespace dfv::raii
