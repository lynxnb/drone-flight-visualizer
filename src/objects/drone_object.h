#pragma once

#include <vector>
#include <thread>
#include <chrono>
#include "data_structs.h"
#include "world_object.h"

namespace dfv::objects {

    class DroneObject : public WorldObject {
        std::vector<FlightDataPoint> flightData;

        glm::dvec3 currentPosition;

    public:
        DroneObject(std::vector<FlightDataPoint> flightData) {
        }

        glm::dvec3 getWorldCoordinates(uint64_t timeNs) {
            return currentPosition;

            FlightDataPoint &flightPoint = flightData[timeNs];

            glm::dvec3 coords{flightPoint.lat, flightPoint.lon, flightPoint.alt};
        };
    }
}
