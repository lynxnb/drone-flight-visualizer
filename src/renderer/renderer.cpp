#include <iostream>
#include <thread>

#include "config/config.h"
#include "renderer.h"

namespace dfv {

    using namespace window_config;

    void rendererThread(Context &context) {
        VulkanEngine &engine = context.engine;
        try {
            engine.init(context.glfwWindow, WindowWidth, WindowHeight);

            while (!context.shouldExit())
                engine.draw();
        } catch (const std::exception &e) {
            std::cerr << "The renderer thread ran into an exception: " << e.what() << std::endl;
        }

        engine.cleanup();
    }

    void startRendererThread(Context &context) {
        std::cout << "Starting render thread" << std::endl;
        std::thread t{rendererThread, std::ref(context)};
        t.detach();
    }

} // namespace dfv
