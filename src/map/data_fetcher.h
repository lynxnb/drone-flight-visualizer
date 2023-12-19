#pragma once

#include "flight_data/geo_types.h"
#include "structs/data_structs.h"
#include "vulkan/vk_mesh.h"
#include <string>
#include <vector>

namespace dfv::map {
    void populateElevation(std::vector<structs::Node *> *nodes);

    structs::OsmData fetchOsmData(const std::string &bbox);

    std::vector<std::vector<structs::Node>> createGridSlave(float llLat, float llLon, float urLat, float urLon, float sparsity);

    std::vector<std::vector<structs::Node>> createGridSlaveMock(float llLat, float llLon, float urLat, float urLon, float spacingMeters);

    auto createGrid(structs::DiscreteBox box, std::vector<structs::Node> &drone_path, float sparsity, float box_size, float node_density_coefficient) -> std::vector<std::vector<structs::DiscreteBoxInfo>>;

    void populateElevation(std::vector<structs::Node> &nodes);

    Mesh createMeshArray(std::vector<std::vector<structs::DiscreteBoxInfo>> &box_matrix, float llLatBound, float llLonBound, float urLatBound, float urLonBound, Coordinate initialPosition);

    void PopulateBatchWithElevationOpenElevation(std::vector<std::reference_wrapper<structs::Node *>> &nodes);

    void PopulateBatchWithElevationGoogle(std::vector<std::reference_wrapper<structs::Node *>> &nodes, std::string &googleApiKey);
} // namespace dfv::map
