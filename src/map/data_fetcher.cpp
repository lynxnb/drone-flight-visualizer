#define _USE_MATH_DEFINES
#define NOMINMAX // Disable min and max macros from windows.h
#include "data_fetcher.h"
#include "flight_data/geo_types.h"
#include "vulkan/vk_mesh.h"
#include <chrono>
#include <cmath>
#include <math.h>
#include <cpr/cpr.h>
#include <cstdlib> // Include for getenv
#include <glm/geometric.hpp>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace dfv::map {
    namespace {
        constexpr int BATCH_SIZE_GOOGLE = 500;
        constexpr int BATCH_SIZE = 5000;
    }

    using namespace dfv::structs;
    using namespace rapidjson;

    std::string GetExecutablePath() {
        char path[FILENAME_MAX];
#if defined(_WIN32)
        GetModuleFileName(NULL, path, FILENAME_MAX);
#elif defined(__unix__)
        ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (len != -1) {
            path[len] = '\0';
        } else {
            // Handle error
            std::cerr << "Error obtaining executable path." << std::endl;
        }
#endif
        return std::string(path);
    }

    std::string ReadApiKeyFromFile(const std::string& filePath) {
        std::ifstream file(filePath);
        std::string apiKey;
        if (file.is_open()) {
            std::getline(file, apiKey);
            file.close();
        } else {
            std::cerr << "Unable to open file: " << filePath << std::endl;
        }
        return apiKey;
    }


    void PopulateBatchWithElevationOpenElevation(std::vector<std::reference_wrapper<structs::Node *>> &nodes) {
        int max_iter = 100;
        int i = 0;
        while (i < max_iter) {
            // write locations to json
            StringBuffer s;
            Writer<StringBuffer> writer(s);
            writer.StartArray();
            for (const auto &node : nodes) {
                writer.StartObject();
                writer.Key("latitude");
                writer.Double(node.get()->lat);
                writer.Key("longitude");
                writer.Double(node.get()->lon);
                writer.EndObject();
            }
            writer.EndArray();
            // send request to open-elevation api
            auto startTime = std::chrono::high_resolution_clock::now();
            cpr::Response r = cpr::Post(cpr::Url{
                                                "https://api.open-elevation.com/api/v1/lookup"
            },
                                        cpr::Body{"{\"locations\":" + std::string(s.GetString()) + "}"}, cpr::Header{{"Content-Type", "application/json"}, {"Accept", "application/json"}});
            if (r.status_code == 429) {
                i++;
                std::cerr << "Received Too Many Requests from API, waiting " << i * 100 << "ms" << std::endl;
                std::chrono::milliseconds(i * 100);
                continue;
            }
            if (r.status_code != 200)
                throw std::runtime_error(
                        "Error in response while fetching elevation data with code " + std::to_string(r.status_code));
            std::string text = r.text;
            // std::cout << r.text;

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            // std::cout << "Elevation batch fetched in " << duration.count() << "ms" << std::endl;

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
                    nodes[i].get()->elev = 0;
                    continue;
                }
                nodes[i].get()->elev = result["elevation"].GetDouble();
                // Add logging here to see what's being set
                // std::cout << "Node #" << i << " elevation set to: " << nodes[i].get().elev << std::endl;
            }
            return;
        }
    }

    void PopulateBatchWithElevationGoogle(std::vector<std::reference_wrapper<structs::Node *>> &nodes, std::string &googleApiKey) {
        int max_iter = 100;
        int i = 0;
        while (i < max_iter) {
            // Construct locations string for Google Elevation API
            std::string locationsParam;
            for (const auto &node : nodes) {
                if (!locationsParam.empty())
                    locationsParam += "|";
                locationsParam += std::to_string(node.get()->lat) + "," + std::to_string(node.get()->lon);
            }

            // Send request to Google Elevation API
            auto startTime = std::chrono::high_resolution_clock::now();
            std::string requestUrl = "https://maps.googleapis.com/maps/api/elevation/json?locations=" + locationsParam + "&key=" + std::string(googleApiKey);
            cpr::Response r = cpr::Get(cpr::Url{
                                               requestUrl
            },
                                       cpr::Header{{"Accept", "application/json"}});

            if (r.status_code == 429) {
                i++;
                std::cerr << "Received Too Many Requests from API, waiting " << i * 100 << "ms" << std::endl;
                std::chrono::milliseconds(i * 100);
                continue;
            }
            if (r.status_code != 200)
                throw std::runtime_error(
                        "Error in response while fetching elevation data with code " + std::to_string(r.status_code));
            std::string text = r.text;

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            // Parse response and return elevations
            Document responseData;
            responseData.Parse(text.c_str());
            if (!responseData.IsObject() || !responseData.HasMember("results")) {
                std::cerr << "Invalid JSON format or missing results." << std::endl;
                return;
            }

            const Value &results = responseData["results"];

            for (SizeType j = 0; j < results.Size(); ++j) {
                const Value &result = results[j];
                if (!result.HasMember("elevation")) {
                    std::cerr << "Missing elevation data in result #" << j << std::endl;
                    nodes[j].get()->elev = 0;
                    continue;
                }
                nodes[j].get()->elev = result["elevation"].GetDouble();
            }
            return;
        }
    }

    void populateElevation(std::vector<structs::Node *> *nodes) {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::string execPath = GetExecutablePath();
        std::string directory;
        const size_t last_slash_idx = execPath.rfind('\\');
        if (std::string::npos != last_slash_idx) {
            directory = execPath.substr(0, last_slash_idx);
        }

        std::string apiKeyFilePath = directory + "/google_api_key.txt"; // Assuming the file is named 'google_api_key.txt'
        std::string googleApiKey = ReadApiKeyFromFile(apiKeyFilePath);

        std::cout << "Fetching " << nodes->size() << " Nodes Elevation" << std::endl;
        int batch_size = 0;
        if (googleApiKey.empty()) {
            batch_size = BATCH_SIZE;
        } else {
            batch_size = BATCH_SIZE_GOOGLE;
        }

        int e = 0;
        for (int i = 0; i < nodes->size(); i += batch_size) {
            auto startIter = nodes->begin() + i;
            auto endIter = nodes->begin() + (nodes->size() > i + batch_size ? i + batch_size : nodes->size());

            std::vector<std::reference_wrapper<structs::Node *>> batch(startIter, endIter);

            if (googleApiKey.empty()) {
                PopulateBatchWithElevationOpenElevation(batch);
            } else {
                PopulateBatchWithElevationGoogle(batch, googleApiKey);
            }

             // Ensure PopulateBatchWithElevation is compatible with this change.
            std::chrono::milliseconds(10);
            std::cout << "Batch " << e << "/" << (nodes->size() / batch_size) + 1 << " of Elevation Data Fetched" << std::endl;
            e++;
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout << "Elevation data fetched in " << duration.count() << "ms" << std::endl;
    }

    ///
    /// \param commonNodes
    /// \param sparseNodes
    /// \param mesh
    /// \param orientation 0 for vertical, 1 for horizontal
    void sewBoxesSlave(std::vector<Node> &commonNodes, std::vector<Node> &sparseNodes, Mesh &mesh, int orientation, bool reverseOrder) {
        int sparseIndex = orientation == 1 ? 1 : 0;
        for (int k = 0; k < commonNodes.size() - 1; ++k) {
            if(orientation == 1 && commonNodes[k].lat < sparseNodes[sparseIndex].lat){
                continue;
            }
            if (sparseIndex + 2 > sparseNodes.size()) {
                break;
            }

            if (orientation == 1 && sparseIndex + 3 > sparseNodes.size()) {
                break;
            }

            Node commonNode = commonNodes[k];
            Node sparseNode = sparseNodes[sparseIndex];
            Node nextCommonNode = commonNodes[k + 1];
            Node nextSparseNode = sparseNodes[sparseIndex + 1];
            if (commonNode.lat == nextCommonNode.lat && commonNode.lon == nextCommonNode.lon) {
                continue;
            }
            if (orientation == 1) {
                if (nextCommonNode.lat == nextSparseNode.lat) {
                    if (!reverseOrder) {
                        mesh.indices.push_back(sparseNode.game_node->vertex_index);
                        mesh.indices.push_back(commonNode.game_node->vertex_index);
                        mesh.indices.push_back(nextCommonNode.game_node->vertex_index);
                    } else {
                        mesh.indices.push_back(sparseNode.game_node->vertex_index);
                        mesh.indices.push_back(nextCommonNode.game_node->vertex_index);
                        mesh.indices.push_back(commonNode.game_node->vertex_index);
                    }
                    if (!reverseOrder) {
                        mesh.indices.push_back(sparseNode.game_node->vertex_index);
                        mesh.indices.push_back(nextCommonNode.game_node->vertex_index);
                        mesh.indices.push_back(nextSparseNode.game_node->vertex_index);
                    } else {
                        mesh.indices.push_back(nextCommonNode.game_node->vertex_index);
                        mesh.indices.push_back(sparseNode.game_node->vertex_index);
                        mesh.indices.push_back(nextSparseNode.game_node->vertex_index);
                    }
                    sparseIndex++;
                } else {
                    if (!reverseOrder) {
                        mesh.indices.push_back(sparseNode.game_node->vertex_index);
                        mesh.indices.push_back(commonNode.game_node->vertex_index);
                        mesh.indices.push_back(nextCommonNode.game_node->vertex_index);
                    } else {
                        mesh.indices.push_back(commonNode.game_node->vertex_index);
                        mesh.indices.push_back(sparseNode.game_node->vertex_index);
                        mesh.indices.push_back(nextCommonNode.game_node->vertex_index);
                    }
                }
            }
            if (orientation == 0) {
                if (nextCommonNode.lon != nextSparseNode.lon) {
                    mesh.indices.push_back(sparseNode.game_node->vertex_index);
                    mesh.indices.push_back(commonNode.game_node->vertex_index);
                    mesh.indices.push_back(nextCommonNode.game_node->vertex_index);
                } else {
                    mesh.indices.push_back(sparseNode.game_node->vertex_index);
                    mesh.indices.push_back(commonNode.game_node->vertex_index);
                    mesh.indices.push_back(nextCommonNode.game_node->vertex_index);

                    mesh.indices.push_back(nextCommonNode.game_node->vertex_index);
                    mesh.indices.push_back(nextSparseNode.game_node->vertex_index);
                    mesh.indices.push_back(sparseNode.game_node->vertex_index);

                    sparseIndex++;
                }
            }
        }
        //return triangles;
    }

    /// Create a complex grid around the drone flight path. Creates a 3 blocks wide dense area around the drone and decreases the density of dots by density/(node_density_coefficient^block_distance)
    /// \param box Box that includes all the drone_path points. Behavior is undefined otherwise
    /// \param drone_path Vector of dots where the drone has been. The PATH on the edge of the box is ignored.
    /// \param box_size Size of the chunk. All boxes are squares so the last one might be discarded
    /// \return Node matrix
    auto createGrid(structs::DiscreteBox box, std::vector<structs::Node> &drone_path, double sparsity, double box_size, double node_density_coefficient) -> std::vector<std::vector<structs::DiscreteBoxInfo>> {
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
                for (const auto &node : drone_path) {
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
                if (box_matrix[i][j].is_on_path) {
                    box_matrix[i - 1][j - 1].sparsity = sparsity;
                    box_matrix[i - 1][j].sparsity = sparsity;
                    box_matrix[i - 1][j + 1].sparsity = sparsity;

                    box_matrix[i][j - 1].sparsity = sparsity;
                    box_matrix[i][j].sparsity = sparsity;
                    box_matrix[i][j + 1].sparsity = sparsity;

                    box_matrix[i + 1][j - 1].sparsity = sparsity;
                    box_matrix[i + 1][j].sparsity = sparsity;
                    box_matrix[i + 1][j + 1].sparsity = sparsity;

                    if (box_matrix[i - 1][j - 1].distance != 0)
                        box_matrix[i - 1][j - 1].distance = 1;
                    if (box_matrix[i - 1][j].distance != 0)
                        box_matrix[i - 1][j].distance = 1;
                    if (box_matrix[i - 1][j + 1].distance != 0)
                        box_matrix[i - 1][j + 1].distance = 1;

                    if (box_matrix[i][j - 1].distance != 0)
                        box_matrix[i][j - 1].distance = 1;
                    box_matrix[i][j].distance = 0;
                    if (box_matrix[i][j + 1].distance != 0)
                        box_matrix[i][j + 1].distance = 1;

                    if (box_matrix[i + 1][j - 1].distance != 0)
                        box_matrix[i + 1][j - 1].distance = 1;
                    if (box_matrix[i + 1][j].distance != 0)
                        box_matrix[i + 1][j].distance = 1;
                    if (box_matrix[i + 1][j + 1].distance != 0)
                        box_matrix[i + 1][j + 1].distance = 1;
                }
            }
        }

        int max_iterations = 1000;
        int iter = 0;
        bool no_changes = false;
        while (!no_changes && iter < max_iterations) {
            no_changes = true;
            for (int i = 0; i < box_matrix.size(); i++) {
                for (int j = 0; j < box_matrix[0].size(); j++) {
                    int closest = INT_MAX;
                    bool iter_changes = false;
                    for (int a = 1; a < box_matrix.size(); a++) {
                        if (i - a > 0) {
                            if (box_matrix[i - a][j].distance < closest) {
                                closest = box_matrix[i - a][j].distance + a;
                                iter_changes = true;
                            }
                        }
                        if (i + a < box_matrix.size()) {
                            if (box_matrix[i + a][j].distance < closest) {
                                closest = box_matrix[i + a][j].distance + a;
                                iter_changes = true;
                            }
                        }
                        if (j - a > 0) {
                            if (box_matrix[i][j - a].distance < closest) {
                                closest = box_matrix[i][j - a].distance + a;
                                iter_changes = true;
                            }
                        }
                        if (j + a < box_matrix[i].size()) {
                            if (box_matrix[i][j + a].distance < closest) {
                                closest = box_matrix[i][j + a].distance + a;
                                iter_changes = true;
                            }
                        }
                        if (iter_changes) {
                            break;
                        }
                    }
                    if (box_matrix[i][j].distance <= closest) {
                        continue;
                    }
                    box_matrix[i][j].distance = closest;
                    no_changes = false;
                }
            }
            iter++;
        }

        std::cout << "Distance Matrix for each chunk. Lower means closer to drone path" << std::endl;

        for (int i = 0; i < box_matrix.size(); i++) {
            for (int j = 0; j < box_matrix[0].size(); j++) {
                std::cout << box_matrix[i][j].distance << ",";
            }
            std::cout << std::endl; // New line at the end of each row
        }

        // Set the sparsity for each box
        for (auto &row : box_matrix) {
            for (auto &inode : row) {
                inode.distance = inode.distance > 100 ? 100 : inode.distance;
                inode.sparsity = sparsity / (pow(node_density_coefficient, inode.distance));
                //inode.sparsity = sparsity * inode.distance;
            }
        }

        std::cout << "Sparsity Matrix (1 / Density) for each chunk. Lower means higher density" << std::endl;

        for (int i = 0; i < box_matrix.size(); i++) {
            for (int j = 0; j < box_matrix[0].size(); j++) {
                std::cout << box_matrix[i][j].sparsity << ",";
            }
            std::cout << ";" << std::endl; // New line at the end of each row
        }

        // Fill the boxes with nodes
        std::vector<structs::Node *> nodes;
        for (auto &row : box_matrix) {
            for (auto &ibox : row) {
                ibox.dots = createGridSlave(ibox.box.llLat, ibox.box.llLon, ibox.box.urLat, ibox.box.urLon, ibox.sparsity);
                for (auto &irow : ibox.dots) {
                    for (auto &inode : irow) {
                        nodes.push_back(&inode);

                        inode.elev = 1600;
                    }
                }
            }
        }

        populateElevation(&nodes);

        return box_matrix;
    }

    glm::vec3 calculateTriangleNormal(const Vertex &v1, const Vertex &v2, const Vertex &v3) {
        // Compute two edges of the triangle
        glm::vec3 edge1 = v2.position - v1.position;
        glm::vec3 edge2 = v3.position - v1.position;

        // Calculate the cross product of the edges
        glm::vec3 normal = glm::cross(edge1, edge2);

        // Normalize the normal to ensure it has unit length
        return glm::normalize(normal);
    }

    Mesh createMeshArray(std::vector<std::vector<structs::DiscreteBoxInfo>> &box_matrix, double llLatBound, double llLonBound, double urLatBound, double urLonBound, Coordinate initialPosition) {
        double totalLatWorldLatSpan = urLatBound - llLatBound;
        double totalLonWorldLatSpan = urLonBound - llLonBound;
        Mesh mesh = {};
        std::vector<structs::Triangle> triangles;
        float elevation_scale = 1;

        for (int ii = 0; ii < box_matrix.size(); ++ii) {
            for (int ie = 0; ie < box_matrix[ii].size(); ++ie) {
                structs::DiscreteBoxInfo *box = &box_matrix[ii][ie];

                //create inner box mash
                for (int i = 0; i < box->dots.size() - 1; ++i) {
                    for (int e = 0; e < box->dots[0].size() - 1; ++e) {
                        if (box->dots[i][e].game_node == nullptr) {
                            box->dots[i][e].game_node = new GameNode();
                            box->dots[i][e].game_node->x = (box->dots[i][e].lat - initialPosition.lat) * SCALING_FACTOR;
                            box->dots[i][e].game_node->z = (box->dots[i][e].lon - initialPosition.lon) * SCALING_FACTOR;
                            box->dots[i][e].game_node->y = box->dots[i][e].elev * elevation_scale;
                            mesh.vertices.push_back({
                                    {box->dots[i][e].game_node->x, box->dots[i][e].game_node->y, box->dots[i][e].game_node->z},
                                    {0.f, 0.f, 0.f},
                                    {0.f, 0.f}
                            });
                            box->dots[i][e].game_node->vertex_index = mesh.vertices.size() - 1;
                        }
                        if (box->dots[i][e + 1].game_node == nullptr) {
                            box->dots[i][e + 1].game_node = new GameNode();
                            box->dots[i][e + 1].game_node->x = (box->dots[i][e + 1].lat - initialPosition.lat) * SCALING_FACTOR;
                            box->dots[i][e + 1].game_node->z = (box->dots[i][e + 1].lon - initialPosition.lon) * SCALING_FACTOR;
                            box->dots[i][e + 1].game_node->y = box->dots[i][e + 1].elev * elevation_scale;
                            mesh.vertices.push_back({
                                    {box->dots[i][e + 1].game_node->x, box->dots[i][e + 1].game_node->y, box->dots[i][e + 1].game_node->z},
                                    {0.f, 0.f, 0.f},
                                    {0.f, 0.f}
                            });
                            box->dots[i][e + 1].game_node->vertex_index = mesh.vertices.size() - 1;
                        }
                        if (box->dots[i + 1][e].game_node == nullptr) {
                            box->dots[i + 1][e].game_node = new GameNode();
                            box->dots[i + 1][e].game_node->x = (box->dots[i + 1][e].lat - initialPosition.lat) * SCALING_FACTOR;
                            box->dots[i + 1][e].game_node->z = (box->dots[i + 1][e].lon - initialPosition.lon) * SCALING_FACTOR;
                            box->dots[i + 1][e].game_node->y = box->dots[i + 1][e].elev * elevation_scale;
                            mesh.vertices.push_back({
                                    {box->dots[i + 1][e].game_node->x, box->dots[i + 1][e].game_node->y, box->dots[i + 1][e].game_node->z},
                                    {0.f, 0.f, 0.f},
                                    {0.f, 0.f}
                            });
                            box->dots[i + 1][e].game_node->vertex_index = mesh.vertices.size() - 1;
                        }

                        structs::Triangle triangle(box->dots[i][e].game_node, box->dots[i][e + 1].game_node, box->dots[i + 1][e].game_node);
                        triangles.push_back(triangle);
                        if (i != 0 && i != box->dots.size() - 2 && e != 0 && e != box->dots[0].size() - 2) {
                            mesh.indices.push_back(box->dots[i][e].game_node->vertex_index);
                            mesh.indices.push_back(box->dots[i][e + 1].game_node->vertex_index);
                            mesh.indices.push_back(box->dots[i + 1][e].game_node->vertex_index);
                        }
                    }
                    for (int e = 0; e < box->dots[0].size() - 1; ++e) {
                        if (box->dots[i + 1][e + 1].game_node == nullptr) {
                            box->dots[i + 1][e + 1].game_node = new GameNode();
                            box->dots[i + 1][e + 1].game_node->x = (box->dots[i + 1][e + 1].lat - initialPosition.lat) * SCALING_FACTOR;
                            box->dots[i + 1][e + 1].game_node->z = (box->dots[i + 1][e + 1].lon - initialPosition.lon) * SCALING_FACTOR;
                            box->dots[i + 1][e + 1].game_node->y = box->dots[i + 1][e + 1].elev * elevation_scale;
                            mesh.vertices.push_back({
                                    {box->dots[i + 1][e + 1].game_node->x, box->dots[i + 1][e + 1].game_node->y, box->dots[i + 1][e + 1].game_node->z},
                                    {0.f, 0.f, 0.f},
                                    {0.f, 0.f}
                            });
                            box->dots[i + 1][e + 1].game_node->vertex_index = mesh.vertices.size() - 1;
                        }
                        structs::Triangle triangle(box->dots[i + 1][e].game_node, box->dots[i + 1][e + 1].game_node, box->dots[i][e + 1].game_node);
                        triangles.push_back(triangle);
                        if (i != 0 && i != box->dots.size() - 2 && e != 0 && e != box->dots[0].size() - 2) {
                            mesh.indices.push_back(box->dots[i][e + 1].game_node->vertex_index);
                            mesh.indices.push_back(box->dots[i + 1][e + 1].game_node->vertex_index);
                            mesh.indices.push_back(box->dots[i + 1][e].game_node->vertex_index);
                        }
                    }
                }

                //sew left box
                if (ie > 0) {
                    std::vector<Node> commonNodes;
                    std::vector<Node> aNodes;
                    std::vector<Node> bNodes;
                    commonNodes.reserve((box_matrix)[ii][ie].dots[0].size() + (box_matrix)[ii][ie - 1].dots[0].size());
                    for (auto &row : (box_matrix)[ii][ie].dots) {
                        commonNodes.push_back(row[0]);
                    }
                    for (auto &row : (box_matrix)[ii][ie - 1].dots) {
                        commonNodes.push_back(row.back());
                    }
                    std::sort(commonNodes.begin(), commonNodes.end(), [](structs::Node &a, structs::Node &b) {
                        return a.lat < b.lat;
                    });
                    for (auto &row : (box_matrix)[ii][ie].dots) {
                        aNodes.push_back(row[1]);
                    }
                    for (auto &row : (box_matrix)[ii][ie - 1].dots) {
                        bNodes.push_back(row.rbegin()[1]);
                    }

                    sewBoxesSlave(commonNodes, aNodes, mesh, 1, false);
                    sewBoxesSlave(commonNodes, bNodes, mesh, 1, true);
                }
                //sew top box
                if (ii != 0) {
                    std::vector<Node> commonNodes;
                    std::vector<Node> aNodes;
                    std::vector<Node> bNodes;

                    //commonNodes.reserve((box_matrix)[ii][ie].dots[0].size() + (box_matrix)[ii - 1][ie].dots[0].size());
                    commonNodes.insert(commonNodes.end(), (box_matrix)[ii][ie].dots[0].begin(), (box_matrix)[ii][ie].dots[0].end());
                    commonNodes.insert(commonNodes.end(), (box_matrix)[ii - 1][ie].dots.back().begin(), (box_matrix)[ii - 1][ie].dots.back().end());

                    std::sort(commonNodes.begin(), commonNodes.end(), [](structs::Node &a, structs::Node &b) {
                        return a.lon < b.lon;
                    });

                    aNodes.insert(aNodes.begin(), (box_matrix)[ii][ie].dots[1].begin(), (box_matrix)[ii][ie].dots[1].end());
                    bNodes.insert(bNodes.begin(), (box_matrix)[ii - 1][ie].dots.rbegin()[1].begin(), (box_matrix)[ii - 1][ie].dots.rbegin()[1].end());

                    sewBoxesSlave(commonNodes, aNodes, mesh, 0, false);
                    sewBoxesSlave(commonNodes, bNodes, mesh, 0, true);
                }
            }
        }

        std::vector<uint32_t> vertex_normal_accumulator(mesh.vertices.size(), 0);

        for (int i = 0; i < mesh.indices.size(); i += 3) {
            Vertex &v1 = mesh.vertices[mesh.indices[i]];
            Vertex &v2 = mesh.vertices[mesh.indices[i + 1]];
            Vertex &v3 = mesh.vertices[mesh.indices[i + 2]];

            if(v1.position == v2.position && v2.position == v3.position){
                continue;
            }

            auto normal = calculateTriangleNormal(v1, v2, v3);

            v1.normal += normal;
            vertex_normal_accumulator[mesh.indices[i]]++;
            v2.normal += normal;
            vertex_normal_accumulator[mesh.indices[i + 1]]++;
            v3.normal += normal;
            vertex_normal_accumulator[mesh.indices[i + 2]]++;
        }

        for (int i = 0; i < mesh.vertices.size(); ++i) {
            if(vertex_normal_accumulator[i] != 0){
                mesh.vertices[i].normal /= static_cast<float>(vertex_normal_accumulator[i]);
            }
            mesh.vertices[i].normal = glm::normalize(mesh.vertices[i].normal);
            if(isnan(mesh.vertices[i].normal.y)){
                mesh.vertices[i].normal = {0, 1, 0};
            }
        }
        return mesh;
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
        while (true) {
            // find nodes with the highest latitude not yet discovered
            for (auto &nodesMatrix : allNodes) {
                for (auto &nodesRows : nodesMatrix) {
                    if (nodesRows[0].lat > northernmostcurrent && nodesRows[0].lat < northernmostupper) {
                        northernmostcurrent = nodesRows[0].lat;
                    }
                }
            }

            // exit condition on monotonicity
            if (lastnorthernmostcurrent == northernmostcurrent)
                break;

            northernmostupper = northernmostcurrent;
            northernmostcurrent = -100;

            // collect the rows with the nodes with the highest latitude
            std::vector<std::vector<structs::Node>> sameLatitudeNodes;
            for (auto &nodesMatrix : allNodes) {
                for (auto &nodesRows : nodesMatrix) {
                    if (nodesRows[0].lat == northernmostupper) {
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
            while (true) {
                double iteration_lower = 400;
                int lowerEl = -1;
                for (int j = 0; j < sameLatitudeNodesIndexes.size(); ++j) {
                    if (sameLatitudeNodes[j].size() > sameLatitudeNodesIndexes[j])
                        if (sameLatitudeNodes[j][sameLatitudeNodesIndexes[j]].lon < iteration_lower && sameLatitudeNodes[j][sameLatitudeNodesIndexes[j]].lon > lower) {
                            iteration_lower = sameLatitudeNodes[j][sameLatitudeNodesIndexes[j]].lon;
                            lowerEl = j;
                        }
                }
                if (iteration_lower == 400)
                    break;
                row.push_back(sameLatitudeNodes[lowerEl][sameLatitudeNodesIndexes[lowerEl]]);
                lower = iteration_lower;
                sameLatitudeNodesIndexes[lowerEl]++;
            }
            nodesBox.push_back(row);
            lastnorthernmostcurrent = northernmostcurrent;
        }
        return nodesBox;
    }

    std::vector<std::vector<structs::Node>> createGridSlave(double llLat, double llLon, double urLat, double urLon, double sparsity) {
        std::vector<std::vector<structs::Node>> nodes;

        const double baseDensity = 1.0;
        double densityScale = std::sqrt(10000.0 / sparsity);

        double latInnerNodes = std::max(0.0, std::floor(baseDensity * densityScale) - 1);
        double lonInnerNodes = std::max(0.0, std::floor(baseDensity * densityScale) - 1);


        // Total nodes including corners
        double latTotalNodes = latInnerNodes + 2;
        double lonTotalNodes = lonInnerNodes + 2;

        double latStep = (urLon - llLon) / (latTotalNodes - 1);
        double lonStep = (urLon - llLon) / (lonTotalNodes - 1);

        // Iterate through each grid point and create nodes
        for (int i = 0; i < latTotalNodes; i++) {
            std::vector<structs::Node> row;
            for (int j = 0; j < lonTotalNodes; j++) {
                double lat = (i == 0) ? llLat : (i == latTotalNodes - 1) ? urLat
                                                                         : llLat + i * latStep;
                double lon = (j == 0) ? llLon : (j == lonTotalNodes - 1) ? urLon
                                                                         : llLon + j * lonStep;

                // Create a node and add it to the row
                structs::Node node{"node", 0, lat, lon, {}}; // Adjust parameters as needed
                row.push_back(node);
            }
            // Add the row to the nodes matrix
            nodes.push_back(row);
        }

        //std::cout << "Starting block" << std::endl;

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
        for (const auto &row : nodes) {
            for (const auto &node : row) {
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

} // namespace dfv::map
