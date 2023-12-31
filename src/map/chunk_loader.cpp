#include "chunk_loader.h"

#include <iostream>
#include <span>

#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include <glm/geometric.hpp>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <utils/env.h>
#include <utils/exepath.h>
#include <utils/time_types.h>
#include <vulkan/vk_mesh.h>

namespace dfv {
    ChunkLoader::ChunkLoader(const int pointCount, const FlightBoundingBox &bbox, const Coordinate &initialPosition)
        : pointCount(pointCount), bbox(bbox), initialPosition(initialPosition) {}

    std::vector<Coordinate> ChunkLoader::generateGrid() const {
        std::vector<Coordinate> coordinates;
        coordinates.reserve(pointCount * pointCount);

        const double latSpacing = (bbox.urLat - bbox.llLat) / (pointCount - 1);
        const double lonSpacing = (bbox.urLon - bbox.llLon) / (pointCount - 1);

        for (int i = 0; i < pointCount; i++) {
            for (int j = 0; j < pointCount; j++) {
                coordinates.emplace_back(bbox.llLat + latSpacing * i,
                                         bbox.llLon + lonSpacing * j,
                                         0);
            }
        }

        return coordinates;
    }

    static void fetchAndPopulateElevationGoogle(std::vector<Coordinate> &coordinates, const std::string &apiKey) {
        // Construct locations string for Google Elevation API
        std::string locationsParam;
        for (const auto &coord : coordinates) {
            locationsParam += std::to_string(coord.lat) + "," + std::to_string(coord.lon) + "|";
        }
        locationsParam.pop_back();

        // Send request to Google Elevation API
        auto start = clock::now();
        const std::string requestUrl = "https://maps.googleapis.com/maps/api/elevation/json?locations=" + locationsParam + "&key=" + std::string(apiKey);
        const cpr::Response response = cpr::Get(cpr::Url(requestUrl),
                                                cpr::Header{
                                                        {"Accept", "application/json"}
        });
        auto end = clock::now();
        std::cout << "Elevation data request (Google) for " << coordinates.size() << " coordinates took " << duration_cast<milliseconds>(end - start) << std::endl;

        if (response.status_code != 200)
            throw std::runtime_error("Elevation data request returned with error code: " + std::to_string(response.status_code));

        // Error check the response
        rapidjson::Document responseData;
        responseData.Parse(response.text.c_str());

        if (!responseData.IsObject()) {
            std::cerr << "Response JSON is in an unexpected format" << std::endl;
            return;
        }
        if (!responseData.HasMember("results")) {
            std::cerr << "Response JSON has no results member" << std::endl;
            return;
        }

        start = clock::now();
        // Populate the input array with elevation data
        const rapidjson::Value &results = responseData["results"];
        for (rapidjson::SizeType i = 0; i < results.Size(); i++) {
            const rapidjson::Value &result = results[i];
            if (!result.HasMember("elevation")) {
                std::cerr << "Skipping invalid response for result #" << i << std::endl;
                continue;
            }
            coordinates[i].alt = result["elevation"].GetDouble();
        }
        end = clock::now();
        std::cout << "Elevation data response parsing took " << duration_cast<milliseconds>(end - start) << std::endl;
    }

    static void fetchAndPopulateElevationOSM(std::vector<Coordinate> &coordinates) {
        // Create the JSON request body
        rapidjson::StringBuffer s;
        rapidjson::Writer writer{s};
        writer.StartArray();
        for (const auto &coord : coordinates) {
            writer.StartObject();
            writer.Key("latitude");
            writer.Double(coord.lat);
            writer.Key("longitude");
            writer.Double(coord.lon);
            writer.EndObject();
        }
        writer.EndArray();

        // Make the request to the Open Elevation API
        auto start = clock::now();
        const cpr::Response response = cpr::Post(cpr::Url("https://api.open-elevation.com/api/v1/lookup"),
                                                 cpr::Body(R"({"locations":)" + std::string(s.GetString()) + "}"),
                                                 cpr::Header{
                                                         {"Content-Type", "application/json"},
                                                         {      "Accept", "application/json"}
        });
        auto end = clock::now();
        std::cout << "Elevation data request (Open Elevation) for " << coordinates.size() << " coordinates took " << duration_cast<milliseconds>(end - start) << std::endl;

        if (response.status_code != 200)
            throw std::runtime_error("Elevation data request returned with error code: " + std::to_string(response.status_code));

        // Error check the response
        rapidjson::Document responseData;
        responseData.Parse(response.text.c_str());

        if (!responseData.IsObject()) {
            std::cerr << "Response JSON is in an unexpected format" << std::endl;
            return;
        }
        if (!responseData.HasMember("results")) {
            std::cerr << "Response JSON has no results member" << std::endl;
            return;
        }

        start = clock::now();
        // Populate the input array with elevation data
        const rapidjson::Value &results = responseData["results"];
        for (rapidjson::SizeType i = 0; i < results.Size(); i++) {
            const auto &result = results[i];

            if (!result.HasMember("elevation") || !result.HasMember("latitude") || !result.HasMember("longitude")) {
                std::cerr << "Skipping invalid response for result #" << i << std::endl;
                continue;
            }

            coordinates[i].alt = result["elevation"].GetDouble();
        }
        end = clock::now();
        std::cout << "Elevation data response parsing took " << duration_cast<milliseconds>(end - start) << std::endl;
    }

    void ChunkLoader::fetchAndPopulateElevation(std::vector<Coordinate> &coordinates) const {
        // Try loading the Google API key from google_api_key.txt
        const std::filesystem::path googleApiKeyPath = getexepath().parent_path() / "google_api_key.txt";
        std::string googleApiKey = env["GOOGLE_API_KEY"];

        if (googleApiKey.empty() && std::filesystem::is_regular_file(googleApiKeyPath)) {
            std::ifstream googleApiKeyFile{googleApiKeyPath};
            std::getline(googleApiKeyFile, googleApiKey);
        }

        if (!googleApiKey.empty()) {
            std::cout << "Fetching map using Google Maps APIs" << std::endl;
            fetchAndPopulateElevationGoogle(coordinates, googleApiKey);
        } else {
            std::cout << "Fetching map using Open Elevations APIs" << std::endl;
            fetchAndPopulateElevationOSM(coordinates);
        }
    }

    /**
     * @brief Creates a mesh object usable by the Vulkan engine from the given vector of coordinates and map loading context.
     * @param coordinates The vector of coordinates to create the mesh from.
     */
    Mesh ChunkLoader::createMesh(const std::vector<Coordinate> &coordinates) const {
        Mesh mesh = {};

        auto start = clock::now();

        // Create the vertex buffer
        mesh.vertices.reserve(coordinates.size());
        for (const auto &coord : coordinates) {
            const auto relativeCoord = calculateRelativePosition(coord, initialPosition);
            const auto u = (coord.lon - bbox.llLon) / (bbox.urLon - bbox.llLon);
            const auto v = 1 - (coord.lat - bbox.llLat) / (bbox.urLat - bbox.llLat);

            mesh.vertices.push_back(Vertex{
                    .position = {relativeCoord.lon, relativeCoord.alt, relativeCoord.lat},
                    .normal = {0, 1, 0},
                    .uv = {u, v}
            });
        }

        auto end = clock::now();
        std::cout << "Vertex buffer creation took " << duration_cast<milliseconds>(end - start) << std::endl;

        start = clock::now();

        // Reserve enough space for 6 indices per square
        const size_t squareCount = coordinates.size() - 2 * pointCount + 1;
        const size_t indexCount = squareCount * 6;
        mesh.indices.reserve(indexCount);

        // Write the indices
        for (uint32_t i = 0; i < pointCount - 1; i++) {
            for (uint32_t j = 0; j < pointCount - 1; j++) {
                // Calculate the indices of the 4 vertices of the square
                const uint32_t topLeft = i * pointCount + j;
                const uint32_t topRight = topLeft + 1;
                const uint32_t bottomLeft = (i + 1) * pointCount + j;
                const uint32_t bottomRight = bottomLeft + 1;

                // Write the indices for the first triangle
                mesh.indices.push_back(topLeft);
                mesh.indices.push_back(bottomLeft);
                mesh.indices.push_back(bottomRight);

                // Write the indices for the second triangle
                mesh.indices.push_back(topLeft);
                mesh.indices.push_back(bottomRight);
                mesh.indices.push_back(topRight);
            }
        }

        end = clock::now();
        std::cout << "Index buffer creation took " << duration_cast<milliseconds>(end - start) << std::endl;

        start = clock::now();

        // Normals first pass: calculate the normals of each triangle and accumulate them in the vertices
        for (uint32_t i = 0; i < mesh.indices.size(); i += 3) {
            const uint32_t i0 = mesh.indices[i];
            const uint32_t i1 = mesh.indices[i + 1];
            const uint32_t i2 = mesh.indices[i + 2];

            const glm::vec3 v1 = mesh.vertices[i1].position - mesh.vertices[i0].position;
            const glm::vec3 v2 = mesh.vertices[i2].position - mesh.vertices[i0].position;

            const glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
            mesh.vertices[i0].normal += normal;
            mesh.vertices[i1].normal += normal;
            mesh.vertices[i2].normal += normal;
        }

        // Normals second pass: normalize the vertex normals
        for (auto &vertex : mesh.vertices) {
            vertex.normal = glm::normalize(vertex.normal);
        }

        end = clock::now();
        std::cout << "Normal buffer creation took " << duration_cast<milliseconds>(end - start) << std::endl;

        return mesh;
    }

    std::vector<std::byte> ChunkLoader::downloadTextureData() const {
        // Get the image based on the bounding box
        const auto latCenter = (bbox.llLat + bbox.urLat) / 2;
        const auto lonCenter = (bbox.llLon + bbox.urLon) / 2;

        const auto latDimension = std::abs(bbox.urLat - bbox.llLat);
        const auto lonDimension = std::abs(bbox.urLon - bbox.llLon);
        const auto dimension = std::max(latDimension, lonDimension);

        auto apiKey = env["NASA_API_KEY"];
        if (apiKey.empty()) {
            std::cerr << "NASA API key not found, falling back to DEMO_KEY" << std::endl;
            apiKey = "DEMO_KEY";
        }

        const auto start = clock::now();
        // clang-format off
        const cpr::Response response = cpr::Get(cpr::Url("https://api.nasa.gov/planetary/earth/imagery"),
                                                cpr::Parameters{
                                                        {"lon", std::to_string(lonCenter)},
                                                        {"lat", std::to_string(latCenter)},
                                                        {"dim", std::to_string(dimension)},
                                                        {"date", "2021-01-01"},
                                                        {"api_key", apiKey},
                                                });
        // clang-format on
        const auto end = clock::now();
        std::cout << "Map texture fetching request took " << duration_cast<milliseconds>(end - start) << std::endl;

        if (response.status_code != 200) {
            std::cerr << "Texture data request returned with error code: " << response.status_code << std::endl;
            return {};
        }

        std::span text{reinterpret_cast<const std::byte *>(response.text.data()), response.text.size()};
        return {text.begin(), text.end()};
    }
} // namespace dfv
