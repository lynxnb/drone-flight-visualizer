#pragma once

#include <vector>
#include <string>
#include <objects/objects.h>

namespace dfv::utils {
    std::vector<objects::FlightDataPoint> readFlightData(const std::string &csvPath);
    void populateElevation(std::vector<objects::Code> &nodes);
    objects::OsmData fetchOsmData(const std::string &bbox);
    std::vector<char> readFile(const std::string &filename);

}
