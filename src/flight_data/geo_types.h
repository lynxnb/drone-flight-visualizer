#pragma once

#include <glm/vec2.hpp>

namespace dfv {
    /**
     * @brief A coordinate in latitude, longitude and altitude.
     */
    struct Coordinate {
        double lat;
        double lon;
        double alt; //!< Altitude in meters
    };

    constexpr auto SCALING_FACTOR = 100000; //  0.00001 = 1.11 meter

    inline glm::vec2 calculateRelativePosition(glm::dvec2 position, glm::dvec2 inRelationTo) {

        auto x = (position.x - inRelationTo.x) * SCALING_FACTOR;
        auto y = (position.y - inRelationTo.y) * SCALING_FACTOR;

        return {x, y};
    }
} // namespace dfv
