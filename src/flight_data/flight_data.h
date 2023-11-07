#pragma once

#include <cstdint>
#include <vector>

#include "geo_types.h"

namespace dfv {
    /**
     * @brief A struct representing a single point of flight data.
     * @details Units are in meters and radians.
     * @details Timestamps should be monotonically increasing and start from 0.
     */
    struct FlightDataPoint {
        float timestamp; //!< Timestamp in seconds
        float x;
        float y;
        float z;
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
        virtual FlightDataPoint getPoint(uint64_t timestamp) = 0;

        virtual uint64_t getDuration() = 0;
        virtual uint64_t getStartTime() = 0;
        virtual uint64_t getEndTime() = 0;

        virtual FlightBoundingBox getBoundingBox() = 0;
    };
} // namespace dfv
