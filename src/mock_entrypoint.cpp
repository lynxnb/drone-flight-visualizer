#include <iostream>

#include <glm/gtx/transform.hpp>

#include "config/config.h"
#include "flight_data/mock_flight_data.h"
#include "glfw/glfw.h"
#include "glfw/glfw_surface.h"
#include "visualizer.h"

class MockVisualizer : public dfv::Visualizer {
  public:
    using dfv::Visualizer::Visualizer; // Inherit constructors

    void loadInitialScene() override {
        auto monkeyMesh = engine.createMesh("monkey", "assets/monkey_smooth.obj");
        auto defaultMaterial = engine.getMaterial("defaultmesh");

        auto monkeyObject = engine.allocateRenderObject();
        monkeyHandle = monkeyObject.handle;
        *monkeyObject.object = {.mesh = monkeyMesh,
                                .material = defaultMaterial,
                                .transform = glm::mat4(1.0f)};

        auto point = data.getPoint(dfv::seconds_f{0});
        monkeyObject.object->transform = glm::translate(glm::mat4(1.0f), glm::vec3(point.x, point.y, point.z));
    }

    void update(dfv::seconds_f deltaTime) override {
        auto monkey = engine.getRenderObject(monkeyHandle);

        time += deltaTime;
        auto point = data.getPoint(time);
        monkey->transform = glm::translate(glm::mat4(1.0f), glm::vec3(point.x, point.y, point.z)) *
                            glm::rotate(glm::mat4(1.0f), point.yaw, glm::vec3(0.0f, 1.0f, 0.0f)) *
                            glm::rotate(glm::mat4(1.0f), point.pitch, glm::vec3(1.0f, 0.0f, 0.0f)) *
                            glm::rotate(glm::mat4(1.0f), point.roll, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    dfv::RenderHandle monkeyHandle{};
    dfv::seconds_f time{};
};

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

    MockVisualizer visualizer{surface, data};
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
