#pragma once

#include <future>
#include <optional>

#include <flight_data/flight_data.h>
#include <vulkan/vk_mesh.h>

namespace dfv {
    class MapManager {
      public:
        /**
         * @brief Starts loading the map in the background.
         */
        void startLoad(FlightData &flightData);

        /**
         * @brief Returns the map mesh if it is ready, or an empty optional otherwise.
         */
        std::optional<Mesh> getMapMesh();

      private:
        std::future<Mesh> mapMeshFuture;
    };
} // namespace dfv
