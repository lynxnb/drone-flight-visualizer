#pragma once

#include "flight_data.h"

#include <filesystem>

namespace dfv {
    class DroneFlightData : public FlightData {
      public:
        explicit DroneFlightData(std::filesystem::path  path);

        bool load() override;

        Coordinate getInitialPosition() override;
        FlightDataPoint getPoint(uint64_t timestamp) override;

        uint64_t getDuration() override;
        uint64_t getStartTime() override;
        uint64_t getEndTime() override;

        FlightBoundingBox getBoundingBox() override;

      private:
        const std::filesystem::path path;
    };
} // namespace dfv
