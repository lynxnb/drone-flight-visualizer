#pragma once
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace dfv::structs {
    struct GameNode {
        double x;
        double y;
        double z = 0;
        unsigned long long vertex_index = -1;
    };


    struct Node {
        std::string type;
        int64_t id;
        double lat;
        double lon;
        double elev = 0;
        GameNode *game_node = nullptr;
        std::map<std::string, std::string> tags;

        Node(double lat, double lon, double elev) :
              id(0), lat(lat), lon(lon), elev(elev) {}

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

    struct Triangle {
        GameNode *a;
        GameNode *b;
        GameNode *c;
    };

    struct DiscreteBox {
        double llLat = 10000;
        double llLon = 10000;
        double urLat = -10000;
        double urLon = -10000;
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