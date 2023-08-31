#pragma once
#include <map>

namespace dfv::objects {
    struct Code {
        std::string type;
        int64_t id;
        double lat;
        double lon;
        double elev = 0;
        std::map<std::string, std::string> tags;

        Code(std::string type, int64_t id, double lat, double lon, std::map<std::string, std::string> tags) :
                type(std::move(type)), id(id), lat(lat), lon(lon), tags(std::move(tags)) {}
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
        std::vector<Code> nodes;
        std::vector<Way> ways;
    };

    struct FlightDataPoint {
        double flyTime;
        double lat;
        double lon;
        double alt;
        double heading;
        double pitch;
        double roll;
        double yaw;

        FlightDataPoint(double flyTime, double lat, double lon, double alt, double heading, double pitch, double roll, double yaw) :
                flyTime(flyTime), lat(lat), lon(lon), alt(alt), heading(heading), pitch(pitch), roll(roll), yaw(yaw) {}
    };
}