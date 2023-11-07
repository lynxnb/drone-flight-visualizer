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
} // namespace dfv
