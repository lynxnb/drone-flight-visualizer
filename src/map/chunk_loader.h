#pragma once

#include <flight_data/flight_data.h>

namespace dfv {
    struct Mesh;

    /**
     * @brief The chunk loader holds the context for loading a map chunk.
     */
    class ChunkLoader {
      public:
        /**
         * @brief Constructs a new chunk loader.
         * @param pointCount The number of points in each dimension of the grid.
         * @param bbox The bounding box of the map.
         * @param initialPosition The initial position of the flying object.
         */
        ChunkLoader(int pointCount, const FlightBoundingBox &bbox, const Coordinate &initialPosition);

        /**
         * @brief Generates a grid of coordinates from the given map loading context.
         * @return A vector of coordinates representing the grid.
         */
        std::vector<Coordinate> generateGrid() const;

        /**
         * @brief Fetches elevation data from the Open Elevation API and populates the given vector of coordinates with it.
         * @param coordinates The vector of coordinates to populate with elevation data.
         */
        void fetchAndPopulateElevation(std::vector<Coordinate> &coordinates) const;

        /**
         * @brief Creates a mesh object usable by the Vulkan engine from the given vector of coordinates and map loading context.
         * @param coordinates The vector of coordinates to create the mesh from.
         */
        Mesh createMesh(const std::vector<Coordinate> &coordinates) const;

      private:
        const int pointCount; //!< The number of points in each dimension of the grid
        const FlightBoundingBox &bbox; //!< The bounding box of the map
        const Coordinate &initialPosition; //!< The initial position of the flying object
    };
} // namespace dfv
