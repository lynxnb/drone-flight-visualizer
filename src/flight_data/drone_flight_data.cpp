#include "drone_flight_data.h"

#include <utility>

namespace dfv {
    DroneFlightData::DroneFlightData(std::filesystem::path path)
        : path(std::move(path)) {}

    bool DroneFlightData::load() {
        return true;
    }

    Coordinate DroneFlightData::getInitialPosition() {
        return {};
    }

    FlightDataPoint DroneFlightData::getPoint(seconds_f timestamp) {
        return {};
    }

    seconds_f DroneFlightData::getDuration() {
        return seconds_f{0};
    }

    seconds_f DroneFlightData::getStartTime() {
        return seconds_f{0};
    }

    seconds_f DroneFlightData::getEndTime() {
        return seconds_f{0};
    }

    FlightBoundingBox DroneFlightData::getBoundingBox() {
        return {};
    }
} // namespace dfv
