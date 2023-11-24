#include <iostream>

#include "config/config.h"
#include "flight_data/drone_flight_data.h"
#include "glfw/glfw.h"
#include "glfw/glfw_surface.h"
#include "map/data_fetcher.h"
#include "structs/data_structs.h"
#include <vector>

int main(int argc, char **argv) {
    dfv::structs::DiscreteBox box = {0, 0, 10, 10}; // Example values

    // Create a sample drone path
    std::vector<dfv::structs::Node> drone_path = {
            dfv::structs::Node("drone", 1, 5.5, 5.5, {}),
            dfv::structs::Node("drone", 2, 6.5, 6.5, {})
            // Add more nodes as needed
    };

    // Call the createGrid method with some debug values
    double sparsity = 0.5; // Example density
    double box_size = 2; // Example box size
    double node_density_coefficient = 0.8; // Example coefficient

    std::vector<std::vector<dfv::structs::DiscreteBoxInfo>> box_matrix = dfv::map::createGrid(box, drone_path, sparsity, box_size, node_density_coefficient);

    double lrLatBound = box_matrix.back().back().dots.back().back().lat;
    double lrLonBound = box_matrix.back().back().dots.back().back().lon;
    std::vector<dfv::structs::Triangle> mesh_array = dfv::map::createMeshArray(&box_matrix,
                                                                               box_matrix[0][0].dots[0][0].lat,
                                                                               box_matrix[0][0].dots[0][0].lon,
                                                                               lrLatBound,
                                                                               lrLonBound);

    return 0;

}
