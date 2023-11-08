#include <iostream>

#include "config/config.h"
#include "flight_data/mock_flight_data.h"
#include "glfw/glfw.h"
#include "glfw/glfw_surface.h"
#include "visualizer.h"

/*
 * The main entrypoint of the mock visualizer.
 */
int main() {
    // Initialize the flight data object
    dfv::MockFlightData data{};

    using namespace dfv::window_config;
    // GLFW initialization
    dfv::raii::Glfw glfw{WindowWidth, WindowHeight, WindowTitle};
    dfv::GlfwSurface surface{glfw.getWindow()};

    dfv::VisualizerCreateInfo createInfo{.surface = surface,
                                         .flightData = data,
                                         .objectModelPath = "assets/models/monkey_smooth.obj",
                                         .objectScale = 1.f};

    dfv::Visualizer visualizer{createInfo};
    visualizer.start();

    auto lastFrameStart = dfv::clock::now();

    while (!glfwWindowShouldClose(glfw.getWindow())) {
        // Poll input events
        glfwPollEvents();

        auto frameStart = dfv::clock::now();
        auto deltaTime = frameStart - lastFrameStart;
        lastFrameStart = frameStart;

        visualizer.drawFrame(deltaTime);
    }

    auto stats = visualizer.getStats();
    auto updateTimeAvg = duration_cast<dfv::milliseconds>(stats.updateTotalTime / stats.frameCount);
    auto drawTimeAvg = duration_cast<dfv::milliseconds>(stats.drawTotalTime / stats.frameCount);
    auto frameTimeAvg = duration_cast<dfv::milliseconds>((stats.updateTotalTime + stats.drawTotalTime) / stats.frameCount);
    auto fpsAvg = 1000.0 / static_cast<double>(frameTimeAvg.count());

    std::cout << "Average update time: " << updateTimeAvg << std::endl;
    std::cout << "Average draw time: " << drawTimeAvg << std::endl;
    std::cout << "Average frame time: " << frameTimeAvg << std::endl;
    std::cout << "Average FPS: " << fpsAvg << std::endl;

    return 0;
}
