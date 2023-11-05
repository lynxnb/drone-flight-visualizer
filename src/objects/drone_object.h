#pragma once

#include <vector>
#include <thread>
#include <chrono>
#include "structs/data_structs.h"
#include "world_object.h"

namespace dfv::objects {

    class DroneObject : public WorldObject {
        std::vector<structs::FlightDataPoint> flightData;
        RenderObject renderObject;

    public:
        DroneObject(std::vector<structs::FlightDataPoint> flightData) {
        }

        glm::dvec3 getWorldCoordinates(uint64_t timeNs) {
            return currentPosition;

            structs::FlightDataPoint &flightPoint = flightData[timeNs];

            glm::dvec3 coords{flightPoint.lat, flightPoint.lon, flightPoint.alt};
        };
    }
}
