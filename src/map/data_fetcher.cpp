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

    }

    void populateElevation(std::vector<structs::Node> &nodes) {
        auto startTime = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < nodes.size(); i += BATCH_SIZE) {
            auto startIter = nodes.begin() + i;
            auto endIter = nodes.begin() + (nodes.size() > i + BATCH_SIZE ? i + BATCH_SIZE : nodes.size());

            std::vector<std::reference_wrapper<structs::Node>> batch(startIter, endIter);
            PopulateBatchWithElevation(batch); // Ensure PopulateBatchWithElevation is compatible with this change.
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout << "Elevation data fetched in " << duration.count() << "ms" << std::endl;
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

    std::vector<std::vector<structs::Node>> createGridSlave(double llLat, double llLon, double urLat, double urLon, double spacingMeters) {
        std::vector<std::vector<structs::Node>> nodes;

        const double EarthRadius = 6371e3; // Radius of the Earth in meters
        const double meterToLat = 1 / (111320 * std::cos(llLat * M_PI / 180));

        for (double lat = llLat; lat <= urLat; ) {
            std::vector<structs::Node> row; // Create a new row for nodes
            for (double lon = llLon; lon <= urLon; ) {
                row.emplace_back("node", -1, lat, lon, std::map<std::string, std::string>());

                // Calculate the new longitude, adjusting for latitude
                double deltaLon = (spacingMeters / EarthRadius) * (180 / M_PI) / std::cos(lat * M_PI / 180);
                lon += deltaLon; // Increment longitude
            }
            nodes.push_back(row); // Add the new row to the nodes

            // Calculate the new latitude
            double deltaLat = spacingMeters * meterToLat;
            lat += deltaLat; // Increment latitude
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
