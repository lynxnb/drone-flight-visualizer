#pragma once

#include <vector>
#include <objects/objects.h>

namespace dfv::utils {
    std::vector<objects::flight_data_point> ReadFlightData(const std::string &csvPath);
    void PopulateElevation(std::vector<objects::node> &nodes);
    objects::osm_data FetchOSMData(const std::string &bbox);
    static std::vector<char> ReadFile(const std::string &filename);
}
