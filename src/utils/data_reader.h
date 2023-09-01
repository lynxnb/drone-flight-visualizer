#pragma once

#include <vector>
#include <string>
#include "structs/data_structs.h"

namespace dfv::utils {
    std::vector<structs::FlightDataPoint> readFlightData(const std::string &csvPath);
    void populateElevation(std::vector<structs::Node> &nodes);
    structs::OsmData fetchOsmData(const std::string &bbox);
    std::vector<char> readFile(const std::string &filename);
}
