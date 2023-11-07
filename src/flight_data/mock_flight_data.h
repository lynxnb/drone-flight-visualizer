#pragma once

#include "flight_data.h"

#include <array>

namespace dfv {
    class MockFlightData : public FlightData {
      public:
        bool load() override;

        Coordinate getInitialPosition() override;
        FlightDataPoint getPoint(seconds_f timestamp) override;

        seconds_f getDuration() override;
        seconds_f getStartTime() override;
        seconds_f getEndTime() override;

        FlightBoundingBox getBoundingBox() override;
    };
} // namespace dfv
