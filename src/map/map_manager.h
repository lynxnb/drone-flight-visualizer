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
         * @brief Loads a map mesh from the given bounding box.
         * @param bbox The boundaries of the map to load. The map will be loaded from a grid of points within this bounding box.
         * @param initialPosition The initial position of the flying object. Map coordinates will be relative to this position.
         */
        static Mesh loadMap(const FlightBoundingBox &bbox, const Coordinate &initialPosition);

      private:
        std::future<Mesh> mapMeshFuture;
    };
} // namespace dfv
