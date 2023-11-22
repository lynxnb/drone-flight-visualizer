#pragma once

#include <vector>
#include <string>
#include "structs/data_structs.h"

namespace dfv::map{
    void populateElevation(std::vector<structs::Node> &nodes);
    std::vector<std::vector<structs::Node>> createGrid(std::vector<structs::DiscreteBox> boxes);
    structs::OsmData fetchOsmData(const std::string &bbox);
    std::vector<std::vector<structs::Node>> createGridSlave(double llLat, double llLon, double urLat, double urLon, double spacingMeters);
    std::vector<std::vector<structs::Node>> createGridSlaveMock(double llLat, double llLon, double urLat, double urLon, double spacingMeters);
    void populateElevation(std::vector<structs::Node> &nodes);
}
