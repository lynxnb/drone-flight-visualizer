#include "map_manager.h"

#include "map/data_fetcher.h"

namespace dfv {
    void MapManager::startLoad(FlightData &flightData) {
        const auto &dronePath = flightData.getPath();

        const auto bbox = flightData.getBoundingBox();
        const auto initialPos = flightData.getInitialPosition();
        constexpr double BOX_OFFSET = 0.01;

        const dfv::structs::DiscreteBox box = {.llLat = bbox.llLat - BOX_OFFSET,
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

        mapMeshFuture = std::async(std::launch::async, [box, initialPos, pathNodes = std::move(pathNodes)]() mutable {
            constexpr double sparsity = 50;
            constexpr double box_size = 0.002; // Example box size
            constexpr double node_density_coefficient = 0.9; // Example coefficient

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
        if (mapMeshFuture.valid() && mapMeshFuture.wait_for(0ms) == std::future_status::ready)
            return mapMeshFuture.get();

        return std::nullopt;
    }
} // namespace dfv
