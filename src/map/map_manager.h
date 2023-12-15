#pragma once

#include <flight_data/flight_data.h>
#include <vector>

#include <structs/data_structs.h>
#include <vulkan/vk_mesh.h>

namespace dfv {
    class MapManager {
      public:
        Mesh initialize(FlightData &flightData);

      private:
        std::vector<structs::Node> drone_path = {};
    };
} // namespace dfv
