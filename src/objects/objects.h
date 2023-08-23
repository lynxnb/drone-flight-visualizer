#pragma once

#include <glaze/glaze.hpp>
#include <glaze/core/macros.hpp>

namespace dfv::objects {
        struct point {
            double latitude;
            double longitude;
            double elevation;
        };
}

GLZ_META(dfv::objects::point, latitude, longitude, elevation);
