#include "map_manager.h"

#include "map/data_fetcher.h"

namespace dfv {
    void MapManager::startLoad(FlightData &flightData) {
        const auto &dronePath = flightData.getPath();

        const auto bbox = flightData.getBoundingBox();
        const auto initialPos = flightData.getInitialPosition();
        constexpr float BOX_OFFSET = 0.05;

        dfv::structs::DiscreteBox box = {.llLat = bbox.llLat - BOX_OFFSET,
                                         .llLon = bbox.llLon - BOX_OFFSET,
                                         .urLat = bbox.urLat + BOX_OFFSET,
                                         .urLon = bbox.urLon + BOX_OFFSET,
                                         .spacingMeters = 0};
        std::vector<structs::Node> pathNodes;
        pathNodes.reserve(dronePath.size());
        for (const auto &point : dronePath) {
            pathNodes.emplace_back(static_cast<double>(point.z) / SCALING_FACTOR + initialPos.lat,
                                   static_cast<double>(point.x) / SCALING_FACTOR + initialPos.lon,
                                   static_cast<double>(point.y));
        }

        pathNodes = {};
        pathNodes.reserve(dronePath.size());
        pathNodes.emplace_back(0.001 + 46.1949826657642, 0.001 + 9.73403023222216, 10);

        box.llLat = -0.008 + 46.1949826657642;
        box.llLon = -0.008 + 9.73403023222216;
        box.urLat = +0.016 + 46.1949826657642;
        box.urLon = +0.016 + 9.73403023222216;

        mapMeshFuture = std::async(std::launch::async, [box, initialPos, pathNodes = std::move(pathNodes)]() mutable {
            constexpr double sparsity = 100;
            constexpr double box_size = 0.008; // Example box size
            constexpr double node_density_coefficient = 0.5; // Example coefficient

            auto boxMatrix = dfv::map::createGrid(box, pathNodes, sparsity, box_size, node_density_coefficient);

            const double lrLatBound = boxMatrix.back().back().dots.back().back().lat;
            const double lrLonBound = boxMatrix.back().back().dots.back().back().lon;
            return dfv::map::createMeshArray(boxMatrix,
                                             boxMatrix[0][0].dots[0][0].lat,
                                             boxMatrix[0][0].dots[0][0].lon,
                                             lrLatBound,
                                             lrLonBound, initialPos);
        });
    }

    std::optional<Mesh> MapManager::getMapMesh() {
        using namespace std::chrono_literals;
        if (mapMeshFuture.valid() && mapMeshFuture.wait_for(0ms) == std::future_status::ready) {
            try {
                return mapMeshFuture.get();
            } catch (const std::exception &e) {
                std::cout << "Map loading encountered an exception: " << e.what() << std::endl;
            }
        }

        return std::nullopt;
    }
} // namespace dfv
