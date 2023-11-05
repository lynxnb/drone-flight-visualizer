#pragma once

#include "context.h"
#include "structs/data_structs.h"
#include "vulkan/render_object.h"
#include "vulkan/vk_engine.h"
#include <iostream>
namespace dfv::objects {
    using namespace std;
    class Drone {
      public:
        Drone(optional<vector<structs::FlightDataPoint>> flightData, dfv::Mesh *mesh, dfv::Material *material, optional<glm::dvec2> bbLowerBounds);
        RenderObject renderObject;
        void goToNextFlightDataPoint();
        void goToPreviousFlightDataPoint();

      private:
        optional<std::vector<structs::FlightDataPoint>> flightData;
        glm::vec3 initialPosition = {0, 0, 0};
        optional<glm::dvec2> bbLowerBounds;

        int currentFlightDataPointIndex = 0;
    };
}
