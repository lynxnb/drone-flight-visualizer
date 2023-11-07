#pragma once

namespace dfv {
    /**
     * @brief A coordinate in latitude, longitude and altitude.
     */
    struct Coordinate {
        float lat;
        float lon;
        float alt; //!< Altitude in meters
    };
} // namespace dfv