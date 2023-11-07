#pragma once

#include <iostream>

#include <vulkan/vk_engine.h>
#include "structs/data_structs.h"
#include "vulkan/render_object.h"

namespace dfv::objects {
    class Drone {
      public:
        Drone() = default;
        Drone(std::vector<structs::FlightDataPoint> flightData, dfv::Mesh *mesh, dfv::Material *material, std::optional<glm::dvec2> bbLowerBounds);

        void update(seconds_f deltaTime, RenderObject *renderObject);

        void goToNextFlightDataPoint();

        void goToPreviousFlightDataPoint();

      public:
        RenderHandle renderHandle{};

      private:
        std::vector<structs::FlightDataPoint> flightData;
        glm::vec3 initialPosition = {0, 0, 0};

        glm::vec3 position{};
        glm::vec3 orientation{};
        glm::vec3 scale{};

        std::optional<glm::dvec2> bbLowerBounds;

        int currentFlightDataPointIndex = 0;
    };
} // namespace dfv::objects
