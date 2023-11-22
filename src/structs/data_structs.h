#pragma once
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace dfv::structs {
    struct Node {
        std::string type;
        int64_t id;
        double lat;
        double lon;
        double elev = 0;
        std::map<std::string, std::string> tags;

        Node(std::string type, int64_t id, double lat, double lon, std::map<std::string, std::string> tags) :
                type(std::move(type)), id(id), lat(lat), lon(lon), tags(std::move(tags)) {}

        void display() const {
            std::cout << "Type: " << type << std::endl;
            std::cout << "ID: " << id << std::endl;
            std::cout << "Latitude: " << lat << std::endl;
            std::cout << "Longitude: " << lon << std::endl;
            std::cout << "Elevation: " << elev << std::endl;
            std::cout << "Tags: " << std::endl;
            for(const auto& tag : tags) {
                std::cout << "\tKey: " << tag.first << " Value: " << tag.second << std::endl;
            }
        }
    };

    struct DiscreteBox {
        double llLat;
        double llLon;
        double urLat;
        double urLon;
        double spacingMeters;
    };

    struct DiscreteBoxInfo {
        DiscreteBox box;
        bool is_on_path;
        double sparsity = 0;
        int distance = INT_MAX;
        std::vector<std::vector<structs::Node>> dots;
    };

    struct Way {
        std::string type;
        int64_t id;
        std::vector<int64_t> nodes;
        std::map<std::string, std::string> tags;

        Way(std::string type, int64_t id, std::vector<int64_t> nodes, std::map<std::string, std::string> tags) :
                type(std::move(type)), id(id), nodes(std::move(nodes)), tags(std::move(tags)) {}
    };

    struct OsmData {
        std::vector<Node> nodes;
        std::vector<Way> ways;
    };
}