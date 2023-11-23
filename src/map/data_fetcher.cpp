#define _USE_MATH_DEFINES
#define NOMINMAX // Disable min and max macros from windows.h
#include "data_fetcher.h"
#include <chrono>
#include <cpr/cpr.h>
#include <iostream>
#include <math.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace dfv::map {
    namespace {
        constexpr int BATCH_SIZE = 20000;
    }

    using namespace dfv::structs;
    using namespace rapidjson;


    void PopulateBatchWithElevation(std::vector<std::reference_wrapper<structs::Node>> &nodes) {
        int max_iter = 100;
        int i = 0;
        while(i<max_iter){
            // write locations to json
            StringBuffer s;
            Writer<StringBuffer> writer(s);
            writer.StartArray();
            for (const auto &node : nodes) {
                writer.StartObject();
                writer.Key("latitude");
                writer.Double(node.get().lat);
                writer.Key("longitude");
                writer.Double(node.get().lon);
                writer.EndObject();
            }
            writer.EndArray();
            // send request to open-elevation api
            auto startTime = std::chrono::high_resolution_clock::now();
            cpr::Response r = cpr::Post(cpr::Url{
                                                "https://api.open-elevation.com/api/v1/lookup"
            },
                                        cpr::Body{"{\"locations\":" + std::string(s.GetString()) + "}"}, cpr::Header{{"Content-Type", "application/json"}, {"Accept", "application/json"}});
            if (r.status_code == 429){
                i++;
                std::chrono::milliseconds(i*100);
                continue;
            }
            if (r.status_code != 200)
                throw std::runtime_error(
                        "Error in response while fetching elevation data with code " + std::to_string(r.status_code));
            std::string text = r.text;
            std::cout << r.text;

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            std::cout << "Elevation batch fetched in " << duration.count() << "ms" << std::endl;

            // parse response and return elevations
            rapidjson::Document responseData;
            responseData.Parse(text.c_str());
            if (!responseData.IsObject()) {
                std::cout << "Invalid JSON format." << std::endl;
                return;
            }

            if (!responseData.HasMember("results")) {
                std::cout << "Invalid JSON result." << std::endl;
                return;
            }

            const rapidjson::Value &results = responseData["results"];

            for (rapidjson::SizeType i = 0; i < results.Size(); ++i) {
                const rapidjson::Value &result = results[i];
                if (!result.HasMember("elevation") || !result.HasMember("latitude") || !result.HasMember("longitude")) {
                    std::cerr << "Missing data in result #" << i << std::endl;
                    nodes[i].get().elev = 0;
                    continue;
                }
                nodes[i].get().elev = result["elevation"].GetDouble();
                // Add logging here to see what's being set
                std::cout << "Node #" << i << " elevation set to: " << nodes[i].get().elev << std::endl;
            }
            return;
        }

    }

    void populateElevation(std::vector<structs::Node> &nodes) {
        auto startTime = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < nodes.size(); i += BATCH_SIZE) {
            auto startIter = nodes.begin() + i;
            auto endIter = nodes.begin() + (nodes.size() > i + BATCH_SIZE ? i + BATCH_SIZE : nodes.size());

            std::vector<std::reference_wrapper<structs::Node>> batch(startIter, endIter);
            PopulateBatchWithElevation(batch); // Ensure PopulateBatchWithElevation is compatible with this change.
            std::chrono::milliseconds(10);
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout << "Elevation data fetched in " << duration.count() << "ms" << std::endl;
    }

    /// Create a complex grid around the drone flight path. Creates a 3 blocks wide dense area around the drone and decreases the density of dots by density/(node_density_coefficient^block_distance)
    /// \param box Box that includes all the drone_path points. Behavior is undefined otherwise
    /// \param drone_path Vector of dots where the drone has been. The PATH on the edge of the box is ignored.
    /// \param box_size Size of the chunk. All boxes are squares so the last one might be discarded
    /// \return Node matrix
    std::vector<std::vector<structs::DiscreteBoxInfo>> createGrid(structs::DiscreteBox box, std::vector<structs::Node> &drone_path, double sparsity, double box_size, double node_density_coefficient){
        // Calculate the number of boxes in latitude and longitude
        int latBoxes = floor((box.urLat - box.llLat) / box_size);
        int lonBoxes = floor((box.urLon - box.llLon) / box_size);

        std::vector<std::vector<structs::DiscreteBoxInfo>> box_matrix(latBoxes, std::vector<structs::DiscreteBoxInfo>(lonBoxes));

        // Iterate over each box in the matrix
        for (int i = 0; i < latBoxes; i++) {
            for (int j = 0; j < lonBoxes; j++) {
                // Calculate the bounds of the box
                double minLat = box.llLat + i * box_size;
                double maxLat = minLat + box_size;
                double minLon = box.llLon + j * box_size;
                double maxLon = minLon + box_size;

                // Create the box and fill in the information
                structs::DiscreteBoxInfo boxInfo{};
                boxInfo.box = {minLat, minLon, maxLat, maxLon, box.spacingMeters};

                boxInfo.is_on_path = false;
                for (const auto& node : drone_path) {
                    if (node.lat >= minLat && node.lat <= maxLat &&
                        node.lon >= minLon && node.lon <= maxLon) {
                        boxInfo.is_on_path = true;
                        break; // No need to check further if one node is inside the box
                    }
                }

                // Add the boxInfo to the matrix
                box_matrix[i][j] = boxInfo;
            }
        }

        // create the 3 block wide high density area. If a box on path is on the edge meh.
        for (int i = 1; i < box_matrix.size() - 1; i++) {
            for (int j = 1; j < box_matrix[0].size() - 1; j++) {
                if(box_matrix[i][j].is_on_path){
                    box_matrix[i-1][j-1].sparsity = sparsity;
                    box_matrix[i-1][j].sparsity = sparsity;
                    box_matrix[i-1][j+1].sparsity = sparsity;

                    box_matrix[i][j-1].sparsity = sparsity;
                    box_matrix[i][j].sparsity = sparsity;
                    box_matrix[i][j+1].sparsity = sparsity;

                    box_matrix[i+1][j-1].sparsity = sparsity;
                    box_matrix[i+1][j].sparsity = sparsity;
                    box_matrix[i+1][j+1].sparsity = sparsity;

                    if (box_matrix[i-1][j-1].distance != 0) box_matrix[i-1][j-1].distance = 1;
                    if (box_matrix[i-1][j].distance != 0) box_matrix[i-1][j].distance = 1;
                    if (box_matrix[i-1][j+1].distance != 0) box_matrix[i-1][j+1].distance = 1;

                    if (box_matrix[i][j-1].distance != 0) box_matrix[i][j-1].distance = 1;
                    box_matrix[i][j].distance = 0;
                    if (box_matrix[i][j+1].distance != 0) box_matrix[i][j+1].distance = 1;

                    if (box_matrix[i+1][j-1].distance != 0) box_matrix[i+1][j-1].distance = 1;
                    if (box_matrix[i+1][j].distance != 0) box_matrix[i+1][j].distance = 1;
                    if (box_matrix[i+1][j+1].distance != 0) box_matrix[i+1][j+1].distance = 1;
                }
            }
        }

        int max_iterations = 1000;
        int iter = 0;
        bool no_changes = false;
        while(!no_changes && iter < max_iterations){
            no_changes = true;
            for (int i = 0; i < box_matrix.size(); i++) {
                for (int j = 0; j < box_matrix[0].size(); j++) {
                    int closest = INT_MAX;
                    bool iter_changes = false;
                    for (int a = 1; a < box_matrix.size(); a++) {
                        if(i-a>0){
                            if(box_matrix[i-a][j].distance < closest){
                                closest = box_matrix[i-a][j].distance + a;
                                iter_changes = true;
                            }
                        }
                        if(i+a<box_matrix.size()){
                            if(box_matrix[i+a][j].distance < closest){
                                closest = box_matrix[i+a][j].distance + a;
                                iter_changes = true;
                            }
                        }
                        if(j-a>0){
                            if(box_matrix[i][j-a].distance < closest){
                                closest = box_matrix[i][j-a].distance + a;
                                iter_changes = true;
                            }
                        }
                        if(j+a<box_matrix.size()){
                            if(box_matrix[i][j+a].distance < closest){
                                closest = box_matrix[i][j+a].distance + a;
                                iter_changes = true;
                            }
                        }
                        if(iter_changes) {
                            break;
                        }
                    }
                    if(box_matrix[i][j].distance <= closest){
                        continue;
                    }
                    box_matrix[i][j].distance = closest;
                    no_changes = false;
                }
            }
            iter++;
        }

        for (int i = 0; i < box_matrix.size(); i++) {
            for (int j = 0; j < box_matrix[0].size(); j++) {
                std::cout << box_matrix[i][j].distance << ",";
            }
            std::cout << std::endl; // New line at the end of each row
        }

        // Set the sparsity for each box
        for(auto &row: box_matrix){
            for(auto &inode: row){
                inode.sparsity = sparsity / (pow(node_density_coefficient, inode.distance));
            }
        }

        for (int i = 0; i < box_matrix.size(); i++) {
            for (int j = 0; j < box_matrix[0].size(); j++) {
                std::cout << box_matrix[i][j].sparsity << ",";
            }
            std::cout << std::endl; // New line at the end of each row
        }

        // Fill the boxes with nodes
        for(auto &row: box_matrix){
            for(auto &inode: row){
                inode.dots = createGridSlave(inode.box.llLat, inode.box.llLon, inode.box.urLat, inode.box.urLon, inode.sparsity);
                for(auto &irow: inode.dots){
                    populateElevation(irow);
                }
            }
        }

        return box_matrix;

    }

    std::vector<dfv::structs::Triangle> createMeshArray(std::vector<std::vector<structs::DiscreteBoxInfo>> *box_matrix, double ulLatBound, double ulLonBound, double lrLatBound, double lrLonBound) {
        double totalLatWorldLatSpan = ulLatBound - lrLatBound;
        double totalLonWorldLatSpan = lrLonBound - ulLonBound;
        std::vector<structs::Triangle> triangles;

        for(auto &row: *box_matrix){
            for(auto &box: row){
                //create inner box mash
                for (int i = 0; i < box.dots.size() - 1; ++i) {
                    for (int e = 0; e < box.dots[0].size() - 1; ++e) {
                        if(box.dots[i][e].game_node == nullptr){
                            box.dots[i][e].game_node = new GameNode();
                            box.dots[i][e].game_node->x = (box.dots[i][e].lat - ulLatBound) / totalLatWorldLatSpan;
                            box.dots[i][e].game_node->y = (box.dots[i][e].lon - lrLonBound) / totalLonWorldLatSpan;
                            box.dots[i][e].game_node->z = box.dots[i][e].elev;
                        }
                        if(box.dots[i][e+1].game_node == nullptr){
                            box.dots[i][e+1].game_node = new GameNode();
                            box.dots[i][e+1].game_node->x = (box.dots[i][e].lat - ulLatBound) / totalLatWorldLatSpan;
                            box.dots[i][e+1].game_node->y = (box.dots[i][e].lon - lrLonBound) / totalLonWorldLatSpan;
                            box.dots[i][e+1].game_node->z = box.dots[i][e].elev;
                        }
                        if(box.dots[i+1][e].game_node == nullptr){
                            box.dots[i+1][e].game_node = new GameNode();
                            box.dots[i+1][e].game_node->x = (box.dots[i][e].lat - ulLatBound) / totalLatWorldLatSpan;
                            box.dots[i+1][e].game_node->y = (box.dots[i][e].lon - lrLonBound) / totalLonWorldLatSpan;
                            box.dots[i+1][e].game_node->z = box.dots[i][e].elev;
                        }
                        structs::Triangle triangle (box.dots[i][e].game_node, box.dots[i][e+1].game_node, box.dots[i+1][e].game_node);
                        triangles.push_back(triangle);
                    }
                    for (int e = 1; e < (*box_matrix)[0].size() - 1; ++e) {
                        if(box.dots[i+1][e+1].game_node == nullptr){
                            box.dots[i+1][e+1].game_node = new GameNode();
                            box.dots[i+1][e+1].game_node->x = (box.dots[i][e].lat - ulLatBound) / totalLatWorldLatSpan;
                            box.dots[i+1][e+1].game_node->y = (box.dots[i][e].lon - lrLonBound) / totalLonWorldLatSpan;
                            box.dots[i+1][e+1].game_node->z = box.dots[i][e].elev;
                        }
                        structs::Triangle triangle (box.dots[i+1][e].game_node, box.dots[i+1][e+1].game_node, box.dots[i][e+1].game_node);
                        triangles.push_back(triangle);
                    }
                }
            }
        }

        return triangles;
    }


    std::vector<std::vector<structs::Node>> createGrid(std::vector<structs::DiscreteBox> boxes) {
        std::vector<std::vector<std::vector<structs::Node>>> allNodes;
        int i = 0;
        allNodes.reserve(boxes.size());
        for (structs::DiscreteBox box : boxes) {
            allNodes.push_back(createGridSlaveMock(box.llLat, box.llLon, box.urLat, box.urLon, box.spacingMeters));
        }
        std::vector<std::vector<structs::Node>> nodesBox;
        double northernmostcurrent = -100;
        double lastnorthernmostcurrent = -100;
        double northernmostupper = +100;
        while (true){

            // find nodes with the highest latitude not yet discovered
            for (auto &nodesMatrix : allNodes) {
                for(auto &nodesRows: nodesMatrix){
                    if(nodesRows[0].lat > northernmostcurrent && nodesRows[0].lat < northernmostupper) {
                        northernmostcurrent = nodesRows[0].lat;
                    }
                }
            }

            // exit condition on monotonicity
            if(lastnorthernmostcurrent == northernmostcurrent) break;

            northernmostupper = northernmostcurrent;
            northernmostcurrent = -100;

            // collect the rows with the nodes with the highest latitude
            std::vector<std::vector<structs::Node>> sameLatitudeNodes;
            for (auto &nodesMatrix : allNodes) {
                for(auto &nodesRows: nodesMatrix){
                    if(nodesRows[0].lat == northernmostupper) {
                        sameLatitudeNodes.push_back(nodesRows);
                    }
                }
            }
            auto size = sameLatitudeNodes.size();
            std::cout << "size: " << size << "\n";

            // merge the rows with the same latitude
            std::vector<int> sameLatitudeNodesIndexes(sameLatitudeNodes.size(), 0);
            double lower = -400;
            std::vector<structs::Node> row;
            while(true){
                double iteration_lower = 400;
                int lowerEl = -1;
                for (int j = 0; j < sameLatitudeNodesIndexes.size(); ++j) {
                    if(sameLatitudeNodes[j].size() > sameLatitudeNodesIndexes[j])
                        if(sameLatitudeNodes[j][sameLatitudeNodesIndexes[j]].lon < iteration_lower && sameLatitudeNodes[j][sameLatitudeNodesIndexes[j]].lon > lower){
                            iteration_lower = sameLatitudeNodes[j][sameLatitudeNodesIndexes[j]].lon;
                            lowerEl = j;
                        }
                }
                if(iteration_lower == 400) break;
                row.push_back(sameLatitudeNodes[lowerEl][sameLatitudeNodesIndexes[lowerEl]]);
                lower = iteration_lower;
                sameLatitudeNodesIndexes[lowerEl] ++;
            }
            nodesBox.push_back(row);
            lastnorthernmostcurrent = northernmostcurrent;
        }
        return nodesBox;
    }

    std::vector<std::vector<structs::Node>> createGridSlave(double llLat, double llLon, double urLat, double urLon, double sparsity) {
        std::vector<std::vector<structs::Node>> nodes;

        // Calculate the number of inner nodes (not including corners)
        double latInnerNodes = std::max(0.0, std::floor((urLat - llLat) / sparsity) - 1);
        double lonInnerNodes = std::max(0.0, std::floor((urLon - llLon) / sparsity) - 1);

        // Total nodes including corners
        double latTotalNodes = latInnerNodes + 2;
        double lonTotalNodes = lonInnerNodes + 2;

        double latStep = (urLon - llLon) / (latTotalNodes - 1);
        double lonStep = (urLon - llLon) / (lonTotalNodes - 1);

        // Iterate through each grid point and create nodes
        for (int i = 0; i < latTotalNodes; i++) {
            std::vector<structs::Node> row;
            for (int j = 0; j < lonTotalNodes; j++) {
                double lat = (i == 0) ? llLat : (i == latTotalNodes - 1) ? urLat : llLat + i * latStep;
                double lon = (j == 0) ? llLon : (j == lonTotalNodes - 1) ? urLon : llLon + j * lonStep;

                // Create a node and add it to the row
                structs::Node node{"node", 0, lat, lon, {}}; // Adjust parameters as needed
                row.push_back(node);
            }
            // Add the row to the nodes matrix
            nodes.push_back(row);
        }

        std::cout << "Starting block" << std::endl;

        for (int i = 0; i < nodes.size(); i++) {
            for (int j = 0; j < nodes[0].size(); j++) {
                std::cout << nodes[i][j].lat << ":" << nodes[i][j].lon << ",";
            }
            std::cout << "|" << std::endl; // New line at the end of each row
        }

        return nodes;
    }


    std::vector<std::vector<structs::Node>> createGridSlaveMock(double llLat, double llLon, double urLat, double urLon, double spacingMeters) {
        std::vector<std::vector<structs::Node>> nodes;

        // Calculate the middle latitude and longitude
        double midLat = (llLat + urLat) / 2.0;
        double midLon = (llLon + urLon) / 2.0;

        // Create the corners and middle points directly
        // Lower-left corner
        structs::Node llNode("node", -1, llLat, llLon, std::map<std::string, std::string>());
        // Lower-right corner
        structs::Node lrNode("node", -1, llLat, urLon, std::map<std::string, std::string>());
        // Upper-left corner
        structs::Node ulNode("node", -1, urLat, llLon, std::map<std::string, std::string>());
        // Upper-right corner
        structs::Node urNode("node", -1, urLat, urLon, std::map<std::string, std::string>());
        // Center
        structs::Node centerNode("node", -1, midLat, midLon, std::map<std::string, std::string>());
        // Middle points between corners and center
        structs::Node midLower("node", -1, llLat, midLon, std::map<std::string, std::string>());
        structs::Node midLeft("node", -1, midLat, llLon, std::map<std::string, std::string>());
        structs::Node midUpper("node", -1, urLat, midLon, std::map<std::string, std::string>());
        structs::Node midRight("node", -1, midLat, urLon, std::map<std::string, std::string>());

        // Add the points to the nodes vector
        nodes.push_back({ulNode, midUpper, urNode}); // Top row
        nodes.push_back({midLeft, centerNode, midRight}); // Middle row
        nodes.push_back({llNode, midLower, lrNode}); // Bottom row

        std::cout << "Box: \n";
        for (const auto& row : nodes) {
            for (const auto& node : row) {
                node.display();
            }
        }
        std::cout << "\n\n";

        return nodes;
    }

    OsmData fetchOsmData(const std::string &bbox) {
        auto startTime = std::chrono::high_resolution_clock::now();
        std::string overpassQuery = R"([out:json];(node()" + bbox + R"();way()" + bbox + R"(););out body;)";
        cpr::Response r = cpr::Get(cpr::Url{
                                           "http://overpass-api.de/api/interpreter"
        },
                                   cpr::Parameters{{"data", overpassQuery}});
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout << "OSM data fetched in " << duration.count() << "ms" << std::endl;

        if (r.status_code != 200)
            throw std::runtime_error(
                    "Error in response while fetching OSM data with code " + std::to_string(r.status_code));
        std::string text = r.text;

        OsmData osmData;
        rapidjson::Document responseData;
        responseData.Parse(text.c_str());
        if (!responseData.IsObject()) {
            std::cout << "Invalid JSON format." << std::endl;
            return {};
        }

        if (!responseData.HasMember("elements")) {
            std::cout << "Invalid JSON format." << std::endl;
            return {};
        }

        const rapidjson::Value &elements = responseData["elements"];
        for (rapidjson::SizeType i = 0; i < elements.Size(); ++i) {
            const rapidjson::Value &element = elements[i];
            if (!element.HasMember("type") || !element.HasMember("id"))
                continue;
            std::map<std::string, std::string> tagMap;
            if (element.HasMember("tags") && element["tags"].IsArray()) {
                const rapidjson::Value &tags = element["tags"];
                for (Value::ConstMemberIterator it = tags.MemberBegin(); it != tags.MemberEnd(); ++it) {
                    const std::string &key = it->name.GetString();
                    std::string value;
                    if (it->value.IsString())
                        value = it->value.GetString();
                    else if (it->value.IsInt())
                        value = std::to_string(it->value.GetInt64());
                    tagMap[key] = value;
                };
            }

            if (element["type"] == "node") {
                osmData.nodes.emplace_back(element["type"].GetString(), element["id"].GetInt64(),
                                           element["lat"].GetDouble(),
                                           element["lon"].GetDouble(), tagMap);
            } else if (element["type"] == "way") {
                if (element["nodes"].IsArray()) {
                    std::vector<int64_t> nodes;
                    for (rapidjson::SizeType j = 0; j < element["nodes"].Size(); ++j) {
                        nodes.push_back(element["nodes"][j].GetInt64());
                    }
                    osmData.ways.emplace_back(element["type"].GetString(), element["id"].GetInt64(), nodes,
                                              tagMap);
                }
                osmData.ways.emplace_back(element["type"].GetString(), element["id"].GetInt64(), std::vector<int64_t>(),
                                          tagMap);
            }
        }
        return osmData;
    }

}
