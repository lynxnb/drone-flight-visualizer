#include <iostream>

#include <glm/gtx/transform.hpp>

#include "config/config.h"
#include "flight_data/drone_flight_data.h"
#include "glfw/glfw.h"
#include "glfw/glfw_surface.h"
#include "objects/drone.h"
#include "visualizer.h"

class DroneVisualizer : public dfv::Visualizer {
  public:
    DroneVisualizer(dfv::GlfwSurface &surface, dfv::DroneFlightData &data)
        : Visualizer(surface, data) {}

    void loadInitialScene() override {
        auto monkeyMesh = engine.createMesh("monkey", "assets/monkey_smooth.obj");
        auto defaultMaterial = engine.getMaterial("defaultmesh");

        auto monkey1Object = engine.allocateRenderObject();
        monkey1 = monkey1Object.handle;
        *monkey1Object.object = {.mesh = monkeyMesh,
                                 .material = defaultMaterial,
                                 .transform = glm::mat4(1.0f)};

        auto monkey2 = engine.allocateRenderObject();
        *monkey2.object = {.mesh = monkeyMesh,
                           .material = defaultMaterial,
                           .transform = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 0.0f, 0.0f))};

        auto monkey3 = engine.allocateRenderObject();
        *monkey3.object = {.mesh = monkeyMesh,
                           .material = defaultMaterial,
                           .transform = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f))};
    }

    void update(dfv::seconds_f deltaTime) override {
        auto monkey = engine.getRenderObject(monkey1);
        monkey->transform = glm::rotate(monkey->transform, 1.f * deltaTime.count(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    dfv::RenderHandle monkey1{};
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

    DroneVisualizer visualizer{surface, data};
    visualizer.init();

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