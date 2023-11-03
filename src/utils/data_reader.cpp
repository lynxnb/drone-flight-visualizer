#include <iostream>
#include <chrono>
#include <rapidjson/document.h>
#include <cpr/cpr.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <csv.hpp>
#include "data_reader.h"


namespace dfv::utils {
    namespace {
        constexpr int BATCH_SIZE = 20000;
    }

    using namespace dfv::structs;
    using namespace rapidjson;


    void PopulateBatch(std::vector<Node> &nodes) {
        // write locations to json
        StringBuffer s;
        Writer<StringBuffer> writer(s);
        writer.StartArray();
        for (const auto &node: nodes) {
            writer.StartObject();
            writer.Key("latitude");
            writer.Double(node.lat);
            writer.Key("longitude");
            writer.Double(node.lon);
            writer.EndObject();
        }
        writer.EndArray();
        // send request to open-elevation api
        auto startTime = std::chrono::high_resolution_clock::now();
        cpr::Response r = cpr::Post(cpr::Url{"https://api.open-elevation.com/api/v1/lookup"},
                                    cpr::Body{"{\"locations\":" + std::string(s.GetString()) + "}"},
                                    cpr::Header{{"Content-Type", "application/json"},
                                                {"Accept",       "application/json"}});
        if (r.status_code != 200)
            throw std::runtime_error(
                    "Error in response while fetching elevation data with code " + std::to_string(r.status_code));
        std::string text = r.text;
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
                nodes[i].elev = 0;
                continue;
            }
            nodes[i].elev = result["elevation"].GetDouble();
        }
    }

    void populateElevation(std::vector<structs::Node> &nodes) {
        auto startTime = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < nodes.size(); i += BATCH_SIZE) {
            auto startIter = nodes.begin() + i;
            auto endIter = nodes.begin() + (nodes.size() > i + BATCH_SIZE ? i + BATCH_SIZE : nodes.size());
            std::vector<Node> batch(startIter, endIter);
            PopulateBatch(batch);
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout << "Elevation data fetched in " << duration.count() << "ms" << std::endl;
    }

    std::vector<structs::Node> createGrid(double llLat, double llLon, double urLat, double urLon, double spacing = 0.0001) {
        std::vector<structs::Node> nodes;
        for (double lat = llLat; lat < urLat; lat += spacing) {
            for (double lon = llLon; lon < urLon; lon += spacing) {
                nodes.emplace_back("node", -1, lat, lon, std::map<std::string, std::string>());
            }
        }
        return nodes;
    }
/*
    ⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⠛⢉⢉⠉⠉⠻⣿⣿⣿⣿⣿⣿
    ⣿⣿⣿⣿⣿⣿⣿⠟⠠⡰⣕⣗⣷⣧⣀⣅⠘⣿⣿⣿⣿⣿
    ⣿⣿⣿⣿⣿⣿⠃⣠⣳⣟⣿⣿⣷⣿⡿⣜⠄⣿⣿⣿⣿⣿
    ⣿⣿⣿⣿⡿⠁⠄⣳⢷⣿⣿⣿⣿⡿⣝⠖⠄⣿⣿⣿⣿⣿
    ⣿⣿⣿⣿⠃⠄⢢⡹⣿⢷⣯⢿⢷⡫⣗⠍⢰⣿⣿⣿⣿⣿
    ⣿⣿⣿⡏⢀⢄⠤⣁⠋⠿⣗⣟⡯⡏⢎⠁⢸⣿⣿⣿⣿⣿
    ⣿⣿⣿⠄⢔⢕⣯⣿⣿⡲⡤⡄⡤⠄⡀⢠⣿⣿⣿⣿⣿⣿
    ⣿⣿⠇⠠⡳⣯⣿⣿⣾⢵⣫⢎⢎⠆⢀⣿⣿⣿⣿⣿⣿⣿
    ⣿⣿⠄⢨⣫⣿⣿⡿⣿⣻⢎⡗⡕⡅⢸⣿⣿⣿⣿⣿⣿⣿
    ⣿⣿⠄⢜⢾⣾⣿⣿⣟⣗⢯⡪⡳⡀⢸⣿⣿⣿⣿⣿⣿⣿
    ⣿⣿⠄⢸⢽⣿⣷⣿⣻⡮⡧⡳⡱⡁⢸⣿⣿⣿⣿⣿⣿⣿
    ⣿⣿⡄⢨⣻⣽⣿⣟⣿⣞⣗⡽⡸⡐⢸⣿⣿⣿⣿⣿⣿⣿
    ⣿⣿⡇⢀⢗⣿⣿⣿⣿⡿⣞⡵⡣⣊⢸⣿⣿⣿⣿⣿⣿⣿
    ⣿⣿⣿⡀⡣⣗⣿⣿⣿⣿⣯⡯⡺⣼⠎⣿⣿⣿⣿⣿⣿⣿
    ⣿⣿⣿⣧⠐⡵⣻⣟⣯⣿⣷⣟⣝⢞⡿⢹⣿⣿⣿⣿⣿⣿
    ⣿⣿⣿⣿⡆⢘⡺⣽⢿⣻⣿⣗⡷⣹⢩⢃⢿⣿⣿⣿⣿⣿
    ⣿⣿⣿⣿⣷⠄⠪⣯⣟⣿⢯⣿⣻⣜⢎⢆⠜⣿⣿⣿⣿⣿
    ⣿⣿⣿⣿⣿⡆⠄⢣⣻⣽⣿⣿⣟⣾⡮⡺⡸⠸⣿⣿⣿⣿
    ⣿⣿⡿⠛⠉⠁⠄⢕⡳⣽⡾⣿⢽⣯⡿⣮⢚⣅⠹⣿⣿⣿
    ⡿⠋⠄⠄⠄⠄⢀⠒⠝⣞⢿⡿⣿⣽⢿⡽⣧⣳⡅⠌⠻⣿
    ⠁⠄⠄⠄⠄⠄⠐⡐⠱⡱⣻⡻⣝⣮⣟⣿⣻⣟⣻⡺⣊*/



    OsmData fetchOsmData(const std::string &bbox) {
        auto startTime = std::chrono::high_resolution_clock::now();
        std::string overpassQuery = R"([out:json];(node()" + bbox + R"();way()" + bbox + R"(););out body;)";
        cpr::Response r = cpr::Get(cpr::Url{"http://overpass-api.de/api/interpreter"},
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

    std::vector<structs::FlightDataPoint> readFlightData(const std::string &csvPath) {

        using namespace csv;
        try {
            auto startTime = std::chrono::high_resolution_clock::now();
            CSVReader reader(csvPath);
            std::vector<structs::FlightDataPoint> flightData;
            for (CSVRow &row: reader) {
                for (CSVField &field: row) {
                    auto heading = !row["OSD.directionOfTravel"].is_null() ? row["OSD.directionOfTravel"].get<double>()
                                                                           : 0;
                    flightData.emplace_back(
                            row["OSD.flyTime [s]"].get<double>(),
                            row["OSD.latitude"].get<double>(),
                            row["OSD.longitude"].get<double>(),
                            row["OSD.altitude [ft]"].get<double>(),
                            heading,
                            row["OSD.pitch"].get<double>(),
                            row["OSD.roll"].get<double>(),
                            row["OSD.yaw"].get<double>());

                }

            }
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            std::cout << "Flight data read in " << duration.count() << "ms" << std::endl;
            return flightData;
        } catch (const std::exception &e) {
            std::cout << "Error while reading flight data: " << e.what() << std::endl;
        }
        return {};
    }

    std::vector<char> readFile(const std::string &filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + filename);
        }
        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }
}

