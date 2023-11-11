#pragma once

#include <GLFW/glfw3.h>

namespace dfv::raii {
    /**
     * @brief Refcounted RAII wrapper for GLFW with support for copy and move semantics.
     */
    class Glfw {
      public:
        Glfw(int width, int height, const char *title) {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Remove once swapchain recreation is implemented

            using namespace dfv::window_config;
            window = glfwCreateWindow(width, height, title, nullptr, nullptr);
            ++instanceCount;
        }

        ~Glfw() {
            if (--instanceCount == 0) {
                glfwDestroyWindow(window);
                glfwTerminate();
            }
        }

        Glfw(const Glfw &other) noexcept
            : window(other.window) {
            ++instanceCount;
        }

        Glfw(Glfw &&other) noexcept
            : window(other.window) {
            other.window = nullptr;
        }

        Glfw &operator=(const Glfw &other) noexcept {
            if (this != &other) {
                window = other.window;
                ++instanceCount;
            }
            return *this;
        }

        Glfw &operator=(Glfw &&other) noexcept {
            if (this != &other) {
                window = other.window;
                other.window = nullptr;
            }
            return *this;
        }

        GLFWwindow *getWindow() {
            return window;
        }

      private:
        GLFWwindow *window = nullptr;

        static inline int instanceCount{0}; //!< The number of instances of Glfw
    };
} // namespace dfv::raii
