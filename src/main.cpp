#include <iostream>

#include "context.h"
#include "renderer/renderer.h"

using namespace dfv;

/*
 * Command line usage: drone_flight_visualizer $1
 * $1: Drone CSV data
 */
int main(int argc, char **argv) {
    std::cout << "drone_flight_visualizer" << std::endl;

    Context context;

    context.inputHandler.addKeyMapping(GLFW_KEY_ESCAPE, [&context](auto action) {
        if (action == Action::Pressed)
            context.setExit();
    });

    auto renderThread = startRenderThread(context);

    // Run the GLFW event loop in the main thread
    while (!context.shouldExit())
        glfwWaitEvents();

    renderThread.join();
    return 0;
}
