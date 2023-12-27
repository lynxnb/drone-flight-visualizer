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
         * @param flightData The flight data to load the map from.
         * @param uniformGrid Whether to load the map from a uniform grid or from the drone's path.
         */
        void startLoad(FlightData &flightData, bool uniformGrid = false);

        /**
         * @brief Returns the map mesh if it is ready, or an empty optional otherwise.
         */
        std::optional<Mesh> getMapMesh();

        /**
         * @brief Returns the map texture if it is ready, or an empty optional otherwise.
         */
        std::optional<std::vector<std::byte>> getMapTexture();

      private:
        std::future<Mesh> mapMeshFuture;
        std::future<std::vector<std::byte>> mapTextureFuture;
    };
} // namespace dfv
