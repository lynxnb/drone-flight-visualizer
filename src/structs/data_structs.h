#pragma once
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace dfv::structs {
    struct GameNode {
        float x;
        float y;
        float z = 0;
        unsigned long long vertex_index = -1;

        std::string toString() const {
            return ("x: " + std::to_string(x) + " y: " + std::to_string(y) + "z: " + std::to_string(z) + " index: " + std::to_string(vertex_index));
        }
    };


    struct Node {
        std::string type;
        int64_t id;
        float lat;
        float lon;
        float elev = 0;
        GameNode *game_node = nullptr;
        std::map<std::string, std::string> tags;

        Node(float lat, float lon, float elev) :
              id(0), lat(lat), lon(lon), elev(elev) {}

        Node(std::string type, int64_t id, float lat, float lon, std::map<std::string, std::string> tags) :
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
        float spacingMeters;
    };

    struct DiscreteBoxInfo {
        DiscreteBox box;
        bool is_on_path;
        float sparsity = 0;
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