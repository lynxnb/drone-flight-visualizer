#pragma once

namespace dfv {
    /**
     * @brief A coordinate in latitude, longitude and altitude.
     */
    struct Coordinate {
        double lat;
        double lon;
        double alt; //!< Altitude in meters
    };

    constexpr double SCALING_FACTOR = 100000.0; //  0.00001 = 1.11 meter

    /**
     * @brief Calculates the relative position of a coordinate in relation to another coordinate.
     * @note The altitude of the returned coordinate is unchanged.
     * @return The relative position of the coordinate, in meters.
     */
    inline Coordinate calculateRelativePosition(const Coordinate &position, const Coordinate &inRelationTo) {
        return {.lat = (position.lat - inRelationTo.lat) * SCALING_FACTOR,
                .lon = (position.lon - inRelationTo.lon) * SCALING_FACTOR,
                .alt = position.alt};
    }
} // namespace dfv
