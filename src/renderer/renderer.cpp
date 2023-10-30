#include <iostream>
#include <thread>

#include "config/config.h"
#include "renderer.h"

namespace dfv {

    using namespace window_config;

    void renderThread(Context &context) {
        VulkanEngine &engine = context.engine;
        try {
            engine.init(context.glfwWindow, WindowWidth, WindowHeight);

            auto lastFrameStart = clock::now();

            while (!context.shouldExit()) {
                auto frameStart = clock::now();
                auto deltaTime = duration_cast<nanoseconds>(frameStart - lastFrameStart);
                lastFrameStart = frameStart;

                engine.update(deltaTime);
                engine.draw();
            }
        } catch (const std::exception &e) {
            std::cerr << "The render thread ran into an exception: " << e.what() << std::endl;
        }

        engine.cleanup();
    }

    std::thread startRenderThread(Context &context) {
        std::cout << "Starting render thread" << std::endl;
        return std::thread{renderThread, std::ref(context)};
    }

} // namespace dfv
