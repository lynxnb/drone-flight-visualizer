#pragma once

#include "flight_data.h"

#include <filesystem>

namespace dfv {
    class DroneFlightData : public FlightData {
      public:
        explicit DroneFlightData(std::filesystem::path path);

        bool load() override;

        Coordinate getInitialPosition() override;
        FlightDataPoint getPoint(seconds_f timestamp) override;
        float getMaximumAltitude() override;
        float getMinimumAltitude() override;

        seconds_f getDuration() override;
        seconds_f getStartTime() override;
        seconds_f getEndTime() override;

        FlightBoundingBox getBoundingBox() override;
        std::vector<FlightDataPoint> &getPath() override;


      private:
        std::vector<FlightDataPoint> loadFlightData(const std::string &csvPath);

        const std::filesystem::path path;
        std::vector<FlightDataPoint> flightDataPoints;
        std::optional<Coordinate> initialPosition;
        FlightBoundingBox boundingBox;
        float maximumAltitude = 0;
        float minimumAltitude = 0;
    };
} // namespace dfv
