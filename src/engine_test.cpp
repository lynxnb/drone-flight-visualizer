#include <iostream>

#include "context.h"
#include "renderer/renderer.h"

using namespace dfv;

/*
 * Command line usage: drone_flight_visualizer $1
 * $1: Drone CSV data
 */
int main(int argc, char **argv) {
    // Vulkan initialization
    std::cout << "engine_test" << std::endl;

    Context context;

    context.inputHandler.addKeyMapping(GLFW_KEY_ESCAPE, [&context](auto action) {
        if (action == Action::Pressed)
            context.setExit();
    });

    auto renderThread = startRenderThread(context);

    std::cout << "Waiting for engine to initialize..." << std::endl;
    while (!context.engine.isInitialized) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::cout << "Engine initialized" << std::endl;

    Material *defaultMeshMaterial = context.engine.getMaterial("defaultmesh");
    Mesh *triangleMesh = context.engine.getMesh("monkey");
    // Add a bunch of monkeys
    for (int x = -20; x <= 20; x++) {
        for (int y = -20; y <= 20; y++) {
            RenderObject monkey = {};
            monkey.mesh = triangleMesh;
            monkey.material = defaultMeshMaterial;
            monkey.position = {x, 0, y};
            monkey.orientation = {0, 0, 0};
            monkey.scale = {0.2, 0.2, 0.2};

            monkey.updateFunc = [&](RenderObject &object, seconds_f deltaTime) {
                object.orientation += glm::vec3{0, -1, 0} * deltaTime.count();
                object.computeTransform();
            };

            context.engine.renderObjects.push_back(monkey);
            glfwPollEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    // Run the GLFW event loop in the main thread
    while (!context.shouldExit())
        glfwWaitEvents();

    renderThread.join();
    return 0;
}
