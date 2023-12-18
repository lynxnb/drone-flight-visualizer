#pragma once

#include <GLFW/glfw3.h>

namespace dfv::raii {
    /**
     * @brief RAII wrapper for GLFW.
     */
    class Glfw {
      public:
        explicit Glfw(const char *title) {
            glfwInit();
            const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

            // Set the window size to 2/3 of the screen size, or 1920x1080 for big screens
            windowedWidth = std::min(mode->width * 2 / 3, 1920);
            windowedHeight = std::min(mode->height * 2 / 3, 1080);
            refreshRate = mode->refreshRate;

            windowedXpos = mode->width / 6;
            windowedYpos = mode->height / 6;

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            // Start as windowed
            windowHandle = glfwCreateWindow(windowedWidth, windowedHeight, title, nullptr, nullptr);
            glfwSetWindowPos(windowHandle, windowedXpos, windowedYpos);
        }

        ~Glfw() {
            glfwDestroyWindow(windowHandle);
            glfwTerminate();
        }

        GLFWwindow *window() const {
            return windowHandle;
        }

        void enterFullscreen() {
            const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
            glfwSetWindowMonitor(windowHandle, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, refreshRate);
            fullscreen = true;
        }

        void exitFullscreen() {
            glfwSetWindowMonitor(windowHandle, nullptr, windowedXpos, windowedYpos, windowedWidth, windowedHeight, refreshRate);
            fullscreen = false;
        }

        void toggleFullscreen() {
            if (!fullscreen)
                enterFullscreen();
            else
                exitFullscreen();
        }

        Glfw(const Glfw &other) = delete;
        Glfw(Glfw &&other) = delete;
        Glfw &operator=(const Glfw &other) = delete;
        Glfw &operator=(Glfw &&other) = delete;

      private:
        GLFWwindow *windowHandle = nullptr;

        bool fullscreen = false;

        int windowedWidth = 0;
        int windowedHeight = 0;
        int refreshRate = 0;

        int windowedXpos = 0;
        int windowedYpos = 0;
    };
} // namespace dfv::raii
