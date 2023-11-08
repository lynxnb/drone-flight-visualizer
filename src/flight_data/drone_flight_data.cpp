#include "drone_flight_data.h"

#include <glm/glm.hpp>

#include <csv.hpp>
#include <utility>

namespace dfv {
    DroneFlightData::DroneFlightData(std::filesystem::path path)
        : path(std::move(path)) {}

    bool DroneFlightData::load() {
        try {
            flightDataPoints = loadFlightData(path.string());
        } catch (const std::exception &e) {
            std::cerr << "Error while loading flight data: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    Coordinate DroneFlightData::getInitialPosition() {
        return initialPosition ? initialPosition.value() : Coordinate{};
    }

    FlightDataPoint DroneFlightData::getPoint(seconds_f timestamp) {
        if(flightDataPoints.empty()){
            std::cerr << "Trying to get point from empty flight data" << std::endl;
            return FlightDataPoint(0,0,0,0,0,0,0);
        }


        auto it = std::lower_bound(flightDataPoints.begin(), flightDataPoints.end(), timestamp,
                                   [](const FlightDataPoint &point, const seconds_f &timestamp) {
                                       return point.timestamp < timestamp.count();
                                   });

        if (it == flightDataPoints.begin()) {
            // targetTimestamp is before the first element in the vector
            return *it;
        } else if (it == flightDataPoints.end()) {
            // targetTimestamp is after the last element in the vector
            return flightDataPoints.back();
        } else {
            // Calculate which point is closer to the targetTimestamp.
            float diffBefore = timestamp.count() - (it - 1)->timestamp;
            float diffAfter = it->timestamp - timestamp.count();
            return (diffBefore < diffAfter) ? *(it - 1) : *it;
        }
    }

    seconds_f DroneFlightData::getDuration() {
        return seconds_f{flightDataPoints.end()->timestamp};
    }

    seconds_f DroneFlightData::getStartTime() {
        return seconds_f{0};
    }

    seconds_f DroneFlightData::getEndTime() {
        return seconds_f{0};
    }

    FlightBoundingBox DroneFlightData::getBoundingBox() {
        return {};
    }

    static glm::vec2 calculateRelativePosition(glm::dvec2 position, glm::dvec2 inRelationTo) {
        const auto scale = 100000; //  0.00001 = 1.11 meter

        auto x = (position.x - inRelationTo.x) * scale;
        auto y = (position.y - inRelationTo.y) * scale;

        return {x, y};
    }

    std::vector<FlightDataPoint> DroneFlightData::loadFlightData(const std::string &csvPath) {
        const auto feetToMeter = 0.3048;
        using namespace csv;

        auto startTime = std::chrono::high_resolution_clock::now();
        CSVReader reader(csvPath);
        std::vector<FlightDataPoint> flightData;

        for (CSVRow &row : reader) {
            // set initial position if not set yet
            if (!initialPosition)
                initialPosition = Coordinate{
                        row["OSD.latitude"].get<double>(),
                        row["OSD.longitude"].get<double>(),
                        row["OSD.altitude [ft]"].get<double>() * feetToMeter};

            // auto heading = !row["OSD.directionOfTravel"].is_null() ? row["OSD.directionOfTravel"].get<double>() : 0;

            // calculate position in relation to initial position
            auto position = calculateRelativePosition(
                    glm::dvec2{row["OSD.latitude"].get<double>(), row["OSD.longitude"].get<double>()},
                    glm::dvec2{initialPosition->lat, initialPosition->lon});

            flightData.emplace_back(
                    row["OSD.flyTime [s]"].get<float>(),
                    position.x,
                    position.y,
                    row["OSD.altitude [ft]"].get<float>() * feetToMeter,
                    row["OSD.pitch"].get<float>(),
                    row["OSD.roll"].get<float>(),
                    row["OSD.yaw"].get<float>());
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout << "Flight data read in " << duration.count() << "ms" << std::endl;
        return flightData;
    }

} // namespace dfv
