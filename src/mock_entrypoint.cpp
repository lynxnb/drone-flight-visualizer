#include <iostream>

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

    // GLFW initialization
    const dfv::raii::Glfw glfw{"Mock Visualizer"};
    dfv::GlfwSurface surface{glfw.window()};

    const dfv::VisualizerCreateInfo createInfo{.surface = surface,
                                               .flightData = data,
                                               .droneModelPath = "assets/models/monkey_smooth.obj",
                                               .droneScale = 1.f};

    dfv::Visualizer visualizer{createInfo};
    visualizer.start();

    auto lastFrameStart = dfv::clock::now();

    while (!glfwWindowShouldClose(glfw.window())) {
        // Poll input events
        glfwPollEvents();

        auto frameStart = dfv::clock::now();
        auto deltaTime = frameStart - lastFrameStart;
        lastFrameStart = frameStart;

        visualizer.drawFrame(deltaTime);
    }

    const auto stats = visualizer.getStats();
    const auto updateTimeAvg = duration_cast<dfv::milliseconds>(stats.updateTotalTime / stats.frameCount);
    const auto drawTimeAvg = duration_cast<dfv::milliseconds>(stats.drawTotalTime / stats.frameCount);
    const auto frameTimeAvg = duration_cast<dfv::milliseconds>((stats.updateTotalTime + stats.drawTotalTime) / stats.frameCount);
    const auto fpsAvg = 1000.0 / static_cast<double>(frameTimeAvg.count());

    std::cout << "Average update time: " << updateTimeAvg << std::endl;
    std::cout << "Average draw time: " << drawTimeAvg << std::endl;
    std::cout << "Average frame time: " << frameTimeAvg << std::endl;
    std::cout << "Average FPS: " << fpsAvg << std::endl;

    return 0;
}
