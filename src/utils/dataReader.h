#pragma once

#include <vector>
#include <objects/objects.h>

namespace dfv::utils {
    void ReadDroneFlightData(const std::string &path);
    void PopulateElevation(std::vector<objects::node> &nodes);
    objects::osm_data FetchOSMData(const std::string &bbox);
}
