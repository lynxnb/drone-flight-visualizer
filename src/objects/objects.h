#pragma once

namespace dfv::objects {
    struct node {
        std::string type;
        int64_t id;
        double lat;
        double lon;
        double elev = 0;
        std::map<std::string, std::string> tags;

        node(std::string type, int64_t id, double lat, double lon, std::map<std::string, std::string> tags) :
                type(std::move(type)), id(id), lat(lat), lon(lon), tags(std::move(tags)) {}
    };

    struct way {
        std::string type;
        int64_t id;
        std::vector<int64_t> nodes;
        std::map<std::string, std::string> tags;

        way(std::string type, int64_t id, std::vector<int64_t> nodes, std::map<std::string, std::string> tags) :
                type(std::move(type)), id(id), nodes(std::move(nodes)), tags(std::move(tags)) {}
    };

    struct osm_data {
        std::vector<node> nodes;
        std::vector<way> ways;
    };
}