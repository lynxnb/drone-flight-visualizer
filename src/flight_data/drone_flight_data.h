#pragma once

#include "flight_data.h"

#include <filesystem>

namespace dfv {
    class DroneFlightData : public FlightData {
      public:
        explicit DroneFlightData(std::filesystem::path  path);

        bool load() override;

        Coordinate getInitialPosition() override;
        FlightDataPoint getPoint(seconds_f timestamp) override;

        seconds_f getDuration() override;
        seconds_f getStartTime() override;
        seconds_f getEndTime() override;

        FlightBoundingBox getBoundingBox() override;

      private:
        const std::filesystem::path path;
    };
} // namespace dfv
