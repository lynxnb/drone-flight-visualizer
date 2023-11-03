#pragma once

#include <vector>
#include <string>
#include "structs/data_structs.h"
#include <vulkan/vk_mesh.h>

namespace dfv::map {
    std::vector<structs::FlightDataPoint> readFlightData(const std::string &csvPath);
    void populateElevation(std::vector<structs::Node> &nodes);
    structs::OsmData fetchOsmData(const std::string &bbox);
    std::vector<char> readFile(const std::string &filename);
    std::vector<Vertex> vertexesFromNodes(const std::vector<structs::Node> &nodes);
}
