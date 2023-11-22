#include <iostream>

#include "config/config.h"
#include "flight_data/drone_flight_data.h"
#include "glfw/glfw.h"
#include "glfw/glfw_surface.h"
#include "map/data_fetcher.h"
#include "structs/data_structs.h"
#include <vector>

int main(int argc, char **argv) {
    std::vector<dfv::structs::DiscreteBox> boxes;
    dfv::structs::DiscreteBox originalBox = {
            .llLat = 46.16555835928235,
            .llLon = 9.742042767088224,
            .urLat = 46.17304729523914,
            .urLon = 9.755868133432079,
            .spacingMeters = 5};
    dfv::structs::DiscreteBox largerBox = {
            .llLat = originalBox.llLat - 0.0045, // 500 meters southward
            .llLon = originalBox.llLon - 0.0070, // 500 meters westward
            .urLat = originalBox.urLat + 0.0045, // 500 meters northward
            .urLon = originalBox.urLon + 0.0070, // 500 meters eastward
            .spacingMeters = 20};
    boxes.push_back(originalBox);
    boxes.push_back(largerBox);
    std::vector<std::vector<dfv::structs::Node>> nodes = dfv::map::createGrid(boxes);
    std::cout << nodes.size();
    for (std::vector<dfv::structs::Node> nodesArr : nodes) {
        dfv::map::populateElevation(nodesArr);
        for (const dfv::structs::Node &node : nodesArr) {
            node.display();
        }
    }

    return 0;
}
