#pragma once

#include "flight_data/geo_types.h"
#include "structs/data_structs.h"
#include "vulkan/vk_mesh.h"
#include <string>
#include <vector>

namespace dfv::map {
    void populateElevation(std::vector<structs::Node *> *nodes);

    structs::OsmData fetchOsmData(const std::string &bbox);

    std::vector<std::vector<structs::Node>> createGridSlave(double llLat, double llLon, double urLat, double urLon, double sparsity);

    std::vector<std::vector<structs::Node>> createGridSlaveMock(double llLat, double llLon, double urLat, double urLon, double spacingMeters);

    auto createGrid(structs::DiscreteBox box, std::vector<structs::Node> &drone_path, double sparsity, double box_size, double node_density_coefficient) -> std::vector<std::vector<structs::DiscreteBoxInfo>>;

    void populateElevation(std::vector<structs::Node> &nodes);

    Mesh createMeshArray(std::vector<std::vector<structs::DiscreteBoxInfo>> &box_matrix, double llLatBound, double llLonBound, double urLatBound, double urLonBound, Coordinate initialPosition);

    void PopulateBatchWithElevationOpenElevation(std::vector<std::reference_wrapper<structs::Node *>> &nodes);

    void PopulateBatchWithElevationGoogle(std::vector<std::reference_wrapper<structs::Node *>> &nodes, std::string &googleApiKey);
} // namespace dfv::map
