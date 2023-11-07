#pragma once

#include <iostream>

#include "flight_data/flight_data.h"
#include "structs/data_structs.h"
#include "vulkan/render_object.h"
#include <vulkan/vk_engine.h>

namespace dfv::objects {
    class Drone {
      public:
        Drone(FlightData &flightData);

        void update(seconds_f deltaTime, RenderObject *renderObject);

        void goToNextFlightDataPoint();

        void goToPreviousFlightDataPoint();

      public:
        RenderHandle renderHandle{};

      private:
        FlightData & flightData;

        glm::vec3 position{};
        glm::vec3 orientation{};
        glm::vec3 scale{};

        seconds_f currentTimeStamp;
    };
} // namespace dfv::objects
