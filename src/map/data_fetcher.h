#pragma once

#include <vector>
#include <string>
#include "structs/data_structs.h"

namespace dfv::map{
    void populateElevation(std::vector<structs::Node> &nodes);
    std::vector<structs::Node> createGrid(double llLat, double llLon, double urLat, double urLon, double spacing = 0.0001);
    structs::OsmData fetchOsmData(const std::string &bbox);
}
