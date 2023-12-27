#include "map_manager.h"

#include "chunk_loader.h"
#include "map/data_fetcher.h"

namespace dfv {
    void MapManager::startLoad(FlightData &flightData, const bool uniformGrid) {
        const auto bbox = flightData.getBoundingBox();
        const auto initialPos = flightData.getInitialPosition();

        if (uniformGrid) {
            constexpr int PointCount = 50;
            constexpr double BboxExpandFactor = 0.05;

            const FlightBoundingBox expandedBbox = {
                    .llLat = bbox.llLat - BboxExpandFactor,
                    .llLon = bbox.llLon - BboxExpandFactor,
                    .urLat = bbox.urLat + BboxExpandFactor,
                    .urLon = bbox.urLon + BboxExpandFactor};

            auto loader = std::make_shared<ChunkLoader>(PointCount, expandedBbox, initialPos);

            mapMeshFuture = std::async(std::launch::async, [loader] {
                std::vector<Coordinate> coordinates = loader->generateGrid();
                loader->fetchAndPopulateElevation(coordinates);

                return loader->createMesh(coordinates);
            });

            mapTextureFuture = std::async(std::launch::async, [loader] {
                return loader->downloadTextureData();
            });
        } else {
            constexpr float BOX_OFFSET = 0.05;
            dfv::structs::DiscreteBox box = {.llLat = bbox.llLat - BOX_OFFSET,
                                             .llLon = bbox.llLon - BOX_OFFSET,
                                             .urLat = bbox.urLat + BOX_OFFSET,
                                             .urLon = bbox.urLon + BOX_OFFSET,
                                             .spacingMeters = 0};

            const auto &dronePath = flightData.getPath();
            std::vector<structs::Node> pathNodes;
            pathNodes.reserve(dronePath.size());
            for (const auto &point : dronePath) {
                pathNodes.emplace_back(static_cast<double>(point.z) / SCALING_FACTOR + initialPos.lat,
                                       static_cast<double>(point.x) / SCALING_FACTOR + initialPos.lon,
                                       static_cast<double>(point.y));
            }

            mapMeshFuture = std::async(std::launch::async, [box, initialPos, pathNodes = std::move(pathNodes)]() mutable {
                constexpr float sparsity = 1;
                constexpr float box_size = 0.02; // Example box size
                constexpr float node_density_coefficient = 0.5; // Example coefficient

                auto boxMatrix = dfv::map::createGrid(box, pathNodes, sparsity, box_size, node_density_coefficient);

                const float lrLatBound = boxMatrix.back().back().dots.back().back().lat;
                const float lrLonBound = boxMatrix.back().back().dots.back().back().lon;
                return dfv::map::createMeshArray(boxMatrix,
                                                 boxMatrix[0][0].dots[0][0].lat,
                                                 boxMatrix[0][0].dots[0][0].lon,
                                                 lrLatBound,
                                                 lrLonBound, initialPos);
            });
        }
    }

    std::optional<Mesh> MapManager::getMapMesh() {
        using namespace std::chrono_literals;
        if (mapMeshFuture.valid() && mapMeshFuture.wait_for(0ms) == std::future_status::ready) {
            try {
                return mapMeshFuture.get();
            } catch (const std::exception &e) {
                std::cerr << "Map mesh loading encountered an exception: " << e.what() << std::endl;
            }
        }

        return std::nullopt;
    }

    std::optional<std::vector<std::byte>> MapManager::getMapTexture() {
        using namespace std::chrono_literals;
        // Wait for the mesh to be ready, even if the texture is ready
        if (!mapMeshFuture.valid() && mapTextureFuture.valid() && mapTextureFuture.wait_for(0ms) == std::future_status::ready) {
            try {
                return mapTextureFuture.get();
            } catch (const std::exception &e) {
                std::cerr << "Map texture loading encountered an exception: " << e.what() << std::endl;
            }
        }

        return std::nullopt;
    }
} // namespace dfv
