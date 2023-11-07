#pragma once

#include <vector>
#include <string>
#include "structs/data_structs.h"
#include <vulkan/vk_mesh.h>

namespace dfv::map {
    std::vector<structs::FlightDataPoint> readFlightData(const std::string &csvPath);
    structs::OsmData fetchOsmData(const std::string &bbox);
    std::vector<char> readFile(const std::string &filename);
    glm::vec2 calculateRelativePosition(glm::dvec2 position, glm::dvec2 inRelationTo);
}
