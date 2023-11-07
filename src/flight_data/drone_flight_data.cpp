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

    FlightDataPoint DroneFlightData::getPoint(uint64_t timestamp) {
        return {};
    }

    uint64_t DroneFlightData::getDuration() {
        return 0;
    }

    uint64_t DroneFlightData::getStartTime() {
        return 0;
    }

    uint64_t DroneFlightData::getEndTime() {
        return 0;
    }

    FlightBoundingBox DroneFlightData::getBoundingBox() {
        return {};
    }
} // namespace dfv
