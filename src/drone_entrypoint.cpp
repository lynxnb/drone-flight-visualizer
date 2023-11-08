#include <iostream>

#include "config/config.h"
#include "flight_data/drone_flight_data.h"
#include "glfw/glfw.h"
#include "glfw/glfw_surface.h"
#include "objects/drone.h"
#include "visualizer.h"

class DroneVisualizer : public dfv::Visualizer {
  public:
    using dfv::Visualizer::Visualizer; // Inherit constructors

    void onStart() override {
        time = flightData.getStartTime();
    }

    void update(dfv::seconds_f deltaTime) override {
        time += deltaTime;
        auto flightPoint = flightData.getPoint(time);
        auto position = glm::vec3{flightPoint.x, flightPoint.y, flightPoint.z};
        std::cout << "Position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
        auto attitude = glm::vec3{flightPoint.pitch, flightPoint.roll, flightPoint.yaw};
        setObjectTransform(position, attitude);
    }

    dfv::seconds_f time{};
};

/*
 * The main entrypoint of the drone flight visualizer.
 *
 * Command line usage: drone_flight_visualizer $1
 * $1: Drone CSV flight_data
 */
int main(int argc, char **argv) {
    std::filesystem::path path;
    if (argc > 1) {
        path = argv[1];

        // Warn about extra arguments
        if (argc > 2) {
            std::cout << "Unused arguments: ";
            for (int i = 2; i < argc; i++)
                std::cout << std::format("'{}' ", argv[i]);
        }
    } else {
        std::cout << "Usage: drone_flight_visualizer $1\n"
                  << "$1: Drone CSV data filepath" << std::endl;
        return 1;
    }

    // Initialize the flight data object
    dfv::DroneFlightData data{path};

    using namespace dfv::window_config;
    // GLFW initialization
    dfv::raii::Glfw glfw{WindowWidth, WindowHeight, WindowTitle};
    dfv::GlfwSurface surface{glfw.getWindow()};

    dfv::VisualizerCreateInfo createInfo{.surface = surface,
                                         .flightData = data,
                                         .objectModelPath = "assets/models/model.obj",
                                         .objectScale = 0.04f};

    DroneVisualizer visualizer{createInfo};
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
