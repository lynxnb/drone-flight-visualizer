#pragma once

#include <vector>
#include <string>
#include "objects/data_structs.h"

namespace dfv::utils {
    std::vector<objects::FlightDataPoint> readFlightData(const std::string &csvPath);
    void populateElevation(std::vector<objects::Node> &nodes);
    objects::OsmData fetchOsmData(const std::string &bbox);
    std::vector<char> readFile(const std::string &filename);
}
