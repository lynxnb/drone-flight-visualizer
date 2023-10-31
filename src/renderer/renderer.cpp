#include <iostream>
#include <thread>

#include "config/config.h"
#include "renderer.h"

namespace dfv {

    using namespace window_config;

    void setCameraInputControls(Context &context) {
        // Camera forward
        context.inputHandler.addKeyMapping(GLFW_KEY_W, [&engine = context.engine](auto action) {
            if (action == Action::Pressed)
                engine.cameraParameters.surgeDirection.fetch_add(1.0f, std::memory_order_relaxed);
            else if (action == Action::Released)
                engine.cameraParameters.surgeDirection.fetch_sub(1.0f, std::memory_order_relaxed);
        });
        // Camera backward
        context.inputHandler.addKeyMapping(GLFW_KEY_S, [&engine = context.engine](auto action) {
            if (action == Action::Pressed)
                engine.cameraParameters.surgeDirection.fetch_sub(1.0f, std::memory_order_relaxed);
            else if (action == Action::Released)
                engine.cameraParameters.surgeDirection.fetch_add(1.0f, std::memory_order_relaxed);
        });

        // Camera left
        context.inputHandler.addKeyMapping(GLFW_KEY_A, [&engine = context.engine](auto action) {
            if (action == Action::Pressed)
                engine.cameraParameters.swayDirection.fetch_sub(1.0f, std::memory_order_relaxed);
            else if (action == Action::Released)
                engine.cameraParameters.swayDirection.fetch_add(1.0f, std::memory_order_relaxed);
        });
        // Camera right
        context.inputHandler.addKeyMapping(GLFW_KEY_D, [&engine = context.engine](auto action) {
            if (action == Action::Pressed)
                engine.cameraParameters.swayDirection.fetch_add(1.0f, std::memory_order_relaxed);
            else if (action == Action::Released)
                engine.cameraParameters.swayDirection.fetch_sub(1.0f, std::memory_order_relaxed);
        });

        // Camera up
        context.inputHandler.addKeyMapping(GLFW_KEY_SPACE, [&engine = context.engine](auto action) {
            if (action == Action::Pressed)
                engine.cameraParameters.heaveDirection.fetch_add(1.0f, std::memory_order_relaxed);
            else if (action == Action::Released)
                engine.cameraParameters.heaveDirection.fetch_sub(1.0f, std::memory_order_relaxed);
        });
        // Camera down
        context.inputHandler.addKeyMapping(GLFW_KEY_LEFT_CONTROL, [&engine = context.engine](auto action) {
            if (action == Action::Pressed)
                engine.cameraParameters.heaveDirection.fetch_sub(1.0f, std::memory_order_relaxed);
            else if (action == Action::Released)
                engine.cameraParameters.heaveDirection.fetch_add(1.0f, std::memory_order_relaxed);
        });

        // Speed up
        context.inputHandler.addKeyMapping(GLFW_KEY_LEFT_SHIFT, [&engine = context.engine](auto action) {
            if (action == Action::Pressed)
                engine.cameraParameters.speedMultiplier.store(2.f, std::memory_order_relaxed);
            else if (action == Action::Released)
                engine.cameraParameters.speedMultiplier.store(1.f, std::memory_order_relaxed);
        });
    }

    void renderThread(Context &context) {
        auto updateTotalTime = nanoseconds{0};
        auto drawTotalTime = nanoseconds{0};

        VulkanEngine &engine = context.engine;
        try {
            engine.init(context.glfwWindow, WindowWidth, WindowHeight);
            setCameraInputControls(context);

            auto lastFrameStart = clock::now();

            while (!context.shouldExit()) {
                auto frameStart = clock::now();
                auto deltaTime = frameStart - lastFrameStart;
                lastFrameStart = frameStart;

                engine.update(deltaTime);
                updateTotalTime += clock::now() - frameStart;

                auto drawStart = clock::now();
                engine.draw();
                drawTotalTime += clock::now() - drawStart;
            }
        } catch (const std::exception &e) {
            std::cerr << "The render thread ran into an exception: " << e.what() << std::endl;
        }

        engine.cleanup();

        auto updateTimeAvg = duration_cast<milliseconds>(updateTotalTime / engine.frameNumber);
        auto drawTimeAvg = duration_cast<milliseconds>(drawTotalTime / engine.frameNumber);
        auto frameTimeAvg = duration_cast<milliseconds>((updateTotalTime + drawTotalTime) / engine.frameNumber);
        auto fpsAvg = 1000.0 / static_cast<double>(frameTimeAvg.count());

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
