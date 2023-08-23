#pragma once

#include <glaze/glaze.hpp>
#include <glaze/core/macros.hpp>

namespace dfv::objects {
    struct point {
        double latitude;
        double longitude;
        double elevation;
    };
    struct location {
        double latitude;
        double longitude;
    };
}

GLZ_META(dfv::objects::location, latitude, longitude);

GLZ_META(dfv::objects::point, latitude, longitude, elevation);
