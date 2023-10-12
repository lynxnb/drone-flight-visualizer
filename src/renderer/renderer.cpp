#include <iostream>
#include <thread>

#include "config.h"
#include "renderer.h"

namespace dfv {

    using namespace render_config;

    void rendererThread(Context &context) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        GLFWwindow *window = glfwCreateWindow((int) WindowWidth, (int) WindowHeight, "Drone Flight Visualizer", nullptr, nullptr);
        glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

        VulkanEngine &engine = context.engine;
        try {
            engine.init(window, WindowWidth, WindowHeight);

            bool enableSelection = true;
            while (!glfwWindowShouldClose(window)) {
                // Handle events on queue
                glfwPollEvents();

                int state = glfwGetKey(window, GLFW_KEY_SPACE);
                if (state == GLFW_PRESS && enableSelection) {
                    engine.selectedShader = (engine.selectedShader + 1) % 2;
                    enableSelection = false;
                } else if (state == GLFW_RELEASE) {
                    enableSelection = true;
                }

                engine.draw();
            }
        } catch (const std::exception &e) {
            std::cerr << "The renderer thread ran into an exception: " << e.what() << std::endl;
        }

        // Notify exit as the window was closed
        context.setExit();

        engine.cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void startRendererThread(Context &context) {
        std::cout << "Starting render thread" << std::endl;
        std::thread t{rendererThread, std::ref(context)};
        t.detach();
    }

} // namespace dfv
