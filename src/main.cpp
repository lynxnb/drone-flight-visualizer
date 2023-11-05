#include <iostream>

#include "context.h"
#include "stdio.h"
#include "map/data_fetcher.h"
#include "renderer/renderer.h"
#include "structs/data_structs.h"

using namespace dfv;

/*
 * Command line usage: drone_flight_visualizer $1
 * $1: Drone CSV data
 */
int main(int argc, char **argv) {

    std::vector<structs::DiscreteBox> boxes;
    structs::DiscreteBox originalBox = {
            .llLat = 46.16555835928235,
            .llLon = 9.742042767088224,
            .urLat = 46.17304729523914,
            .urLon = 9.755868133432079,
            .spacingMeters = 5
    };
    structs::DiscreteBox largerBox = {
            .llLat = originalBox.llLat - 0.0045,  // 500 meters southward
            .llLon = originalBox.llLon - 0.0070,  // 500 meters westward
            .urLat = originalBox.urLat + 0.0045,  // 500 meters northward
            .urLon = originalBox.urLon + 0.0070,  // 500 meters eastward
            .spacingMeters = 20
    };
    boxes.push_back(originalBox);
    boxes.push_back(largerBox);
    std::vector<std::vector<structs::Node>> nodes = map::createGrid(boxes);
    std::cout << nodes.size();
    for (std::vector<structs::Node> nodesArr : nodes) {
        map::populateElevation(nodesArr);
        for (const structs::Node &node : nodesArr){
            node.display();
        }
    }


    return 0;

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
