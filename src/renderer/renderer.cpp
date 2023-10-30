#include <iostream>
#include <thread>

#include "config/config.h"
#include "renderer.h"

namespace dfv {

    using namespace window_config;

    void renderThread(Context &context) {
        auto updateTotalTime = nanoseconds{0};
        auto drawTotalTime = nanoseconds{0};

        VulkanEngine &engine = context.engine;
        try {
            engine.init(context.glfwWindow, WindowWidth, WindowHeight);

            auto lastFrameStart = clock::now();

            while (!context.shouldExit()) {
                auto frameStart = clock::now();
                auto deltaTime = duration_cast<nanoseconds>(frameStart - lastFrameStart);
                lastFrameStart = frameStart;

                engine.update(deltaTime);
                updateTotalTime += duration_cast<nanoseconds>(clock::now() - frameStart);

                auto drawStart = clock::now();
                engine.draw();
                drawTotalTime += duration_cast<nanoseconds>(clock::now() - drawStart);
            }
        } catch (const std::exception &e) {
            std::cerr << "The render thread ran into an exception: " << e.what() << std::endl;
        }

        engine.cleanup();

        auto updateTimeAvg = duration_cast<milliseconds>(updateTotalTime / engine.frameNumber);
        auto drawTimeAvg = duration_cast<milliseconds>(drawTotalTime / engine.frameNumber);
        auto frameTimeAvg = duration_cast<milliseconds>((updateTotalTime + drawTotalTime) / engine.frameNumber);
        auto fpsAvg = 1000.0 / frameTimeAvg.count();

        std::cout << "Average update time: " << updateTimeAvg << std::endl;
        std::cout << "Average draw time: " << drawTimeAvg << std::endl;
        std::cout << "Average frame time: " << frameTimeAvg << std::endl;
        std::cout << "Average FPS: " << fpsAvg << std::endl;
    }

    std::thread startRenderThread(Context &context) {
        std::cout << "Starting render thread" << std::endl;
        return std::thread{renderThread, std::ref(context)};
    }

} // namespace dfv
