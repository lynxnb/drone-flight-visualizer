#include "map_manager.h"

#include "map/data_fetcher.h"

namespace dfv {
    Mesh MapManager::initialize(FlightData &flightData) {
        const auto &drone_path = flightData.getPath();

        auto bbox = flightData.getBoundingBox();

        dfv::structs::DiscreteBox box = {.llLat = bbox.llLat,
                                         .llLon = bbox.llLon,
                                         .urLat = bbox.urLat,
                                         .urLon = bbox.urLon,
                                         .spacingMeters = 0};

        for (const auto &point : drone_path) {
            this->drone_path.emplace_back(static_cast<double>(point.x),
                                          static_cast<double>(point.z),
                                          static_cast<double>(point.y));
        }

        box.llLat -= 0.01;
        box.llLon -= 0.01;
        box.urLat += 0.01;
        box.urLon += 0.01;

        for (auto el : drone_path) {
            this->drone_path.emplace_back(el.x, el.z, el.y);
        }

        // Call the createGrid method with some debug values
        double sparsity = 4; // Example density
        double box_size = 0.002; // Example box size
        double node_density_coefficient = 0.8; // Example coefficient

        std::vector<std::vector<dfv::structs::DiscreteBoxInfo>> box_matrix = dfv::map::createGrid(box, this->drone_path, sparsity, box_size, node_density_coefficient);

        double lrLatBound = box_matrix.back().back().dots.back().back().lat;
        double lrLonBound = box_matrix.back().back().dots.back().back().lon;
        auto mesh_array = dfv::map::createMeshArray(&box_matrix,
                                                    box_matrix[0][0].dots[0][0].lat,
                                                    box_matrix[0][0].dots[0][0].lon,
                                                    lrLatBound,
                                                    lrLonBound, flightData.getInitialPosition());
        return mesh_array;
    }

} // namespace dfv
