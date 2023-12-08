#pragma once

#include "structs/data_structs.h"
#include "vulkan/vk_mesh.h"
#include <string>
#include <vector>

namespace dfv::map{
    void populateElevation(std::vector<structs::Node*> *nodes);
    structs::OsmData fetchOsmData(const std::string &bbox);
    std::vector<std::vector<structs::Node>> createGridSlave(double llLat, double llLon, double urLat, double urLon, double sparsity);
    std::vector<std::vector<structs::Node>> createGridSlaveMock(double llLat, double llLon, double urLat, double urLon, double spacingMeters);
    std::vector<std::vector<structs::DiscreteBoxInfo>> createGrid(structs::DiscreteBox box, std::vector<structs::Node> &drone_path, double sparsity, double box_size, double node_density_coefficient);
    void populateElevation(std::vector<structs::Node> &nodes);
    Mesh createMeshArray(std::vector<std::vector<structs::DiscreteBoxInfo>> *box_matrix, double llLatBound, double llLonBound, double urLatBound, double urLonBound);
}
