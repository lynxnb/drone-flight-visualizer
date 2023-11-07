#pragma once

#include <cstdint>
#include <vector>

#include "geo_types.h"
#include <utils/time_types.h>

namespace dfv {
    /**
     * @brief A struct representing a single point of flight data.
     * @details Units are in meters and radians.
     * @details Timestamps should be monotonically increasing and start from 0.
     *
     * The reference system used has the following properties:                                   y   z
     * - the z-axis points to the north pole                                                     |  /
     * - the y-axis points outwards from the center of the earth                                 | /
     * - the x-axis points perpendicular to the other two                                        0------ x
     */
    struct FlightDataPoint {
        float timestamp; //!< Timestamp in seconds
        float x; //!< Position in the x-axis, corresponds to longitude in our plane
        float y; //!< Position in the y-axis, corresponds to altitude in our plane
        float z; //!< Position in the z-axis, corresponds to latitude in our plane
        float pitch; //!< Angle of rotation about the x-axis
        float yaw; //!< Angle of rotation about the y-axis
        float roll; //!< Angle of rotation about the z-axis
    };

    struct FlightBoundingBox {
        float llLat; //!< Lower left latitude
        float llLon; //!< Lower left longitude
        float urLat; //!< Upper right latitude
        float urLon; //!< Upper right longitude
    };

    /**
     * @brief An interface for a flight data provider.
     */
    class FlightData {
      public:
        FlightData() = default;
        virtual ~FlightData() = default;

        /**
         * @brief Loads the flight data. It is recommended to perform expensive operations here (e.g. parsing a file).
         * @return True if successful, false otherwise.
         */
        virtual bool load() = 0;

        virtual Coordinate getInitialPosition() = 0;
        virtual FlightDataPoint getPoint(seconds_f timestamp) = 0;

        virtual seconds_f getDuration() = 0;
        virtual seconds_f getStartTime() = 0;
        virtual seconds_f getEndTime() = 0;

        virtual FlightBoundingBox getBoundingBox() = 0;
    };
} // namespace dfv
