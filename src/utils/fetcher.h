#pragma once

#include <vector>
#include <objects/objects.h>

namespace dfv::utils {
    using point = dfv::objects::point;

    std::vector<double> FetchElevation(const std::vector<point>& points);
}
