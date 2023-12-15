#include "map_manager.h"

#include "map/data_fetcher.h"

namespace dfv {
    Mesh MapManager::initialize(FlightData &flightData) {
        auto drone_path = flightData.getPath();
        dfv::structs::DiscreteBox box = {}; // Example values
        for (const auto &point : drone_path) {
            if (point.z == 0 || point.x == 0) {
                std::cout << "found null: " << point.x << point.z << std::endl;
            }
            box.llLat = std::min(box.llLat, double(point.x));
            box.llLon = std::min(box.llLon, double(point.z));
            box.urLat = std::max(box.urLat, double(point.x));
            box.urLon = std::max(box.urLon, double(point.z));
            this->drone_path.push_back(structs::Node(double(point.x), double(point.z), double(point.y)));
        }

        box.llLat -= 5;
        box.llLon -= 5;
        box.urLat += 5;
        box.urLon += 5;

        for (auto el : drone_path) {
            this->drone_path.emplace_back(el.x, el.z, el.y);
        }

        // Call the createGrid method with some debug values
        double sparsity = 4; // Example density
        double box_size = 2; // Example box size
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
