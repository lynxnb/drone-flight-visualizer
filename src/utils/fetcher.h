#pragma once

#include <vector>
#include <objects/objects.h>

namespace dfv::utils {

    void PopulateBatch(std::vector<objects::node> &nodes);
    void PopulateElevation(std::vector<objects::node> &nodes);
    objects::osm_data FetchOSMData(const std::string &bbox);
}
