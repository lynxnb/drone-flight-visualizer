#include "drone_flight_data.h"

#include <glm/glm.hpp>

#include <csv.hpp>
#include <utility>
#include <utils/time_types.h>
#define M_PI 3.14159265358979323846f

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
        if (flightDataPoints.empty()) {
            std::cerr << "Trying to get point from empty flight data" << std::endl;
            return FlightDataPoint(0, 0, 0, 0, 0, 0, 0);
        }

        auto pointIt = std::lower_bound(flightDataPoints.begin(), flightDataPoints.end(), timestamp,
                                   [](const FlightDataPoint &point, const seconds_f &timestamp) {
                                       return point.timestamp < timestamp.count();
                                   });

        auto nextPointIt = pointIt != flightDataPoints.end() ? pointIt + 1 : pointIt;

        // Interpolate between the current point and the next one
        float lerpTime = (timestamp.count() - pointIt->timestamp) / (nextPointIt->timestamp - pointIt->timestamp);

        auto lerp = [&](float start, float end) {
            return start + (end - start) * lerpTime;
        };

        auto lerpAngle = [&](float start, float end) {

            float diff = end - start;
            // Lerp the angle in the shortest direction
            if (diff > M_PI)
                diff -= 2 * M_PI;
            else if (diff < -M_PI)
                diff += 2 * M_PI;

            return start + diff * lerpTime;
        };

        return {.timestamp = timestamp.count(),
                .x = lerp(pointIt->x, nextPointIt->x),
                .y = lerp(pointIt->y, nextPointIt->y),
                .z = lerp(pointIt->z, nextPointIt->z),
                .pitch = lerpAngle(pointIt->pitch, nextPointIt->pitch),
                .yaw = lerpAngle(pointIt->yaw, nextPointIt->yaw),
                .roll = lerpAngle(pointIt->roll, nextPointIt->roll)};
    }

    seconds_f DroneFlightData::getDuration() {
        return seconds_f{flightDataPoints.end()->timestamp};
    }

    seconds_f DroneFlightData::getStartTime() {
        return seconds_f{flightDataPoints.begin()->timestamp};
    }

    seconds_f DroneFlightData::getEndTime() {
        return seconds_f{flightDataPoints.end()->timestamp};
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

        auto startTime = clock::now();
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
                    0,
                    position.y,
                    glm::radians(row["OSD.pitch"].get<float>()),
                    glm::radians(row["OSD.yaw"].get<float>()),
                    glm::radians(row["OSD.roll"].get<float>()));
        }
        auto endTime = clock::now();
        auto duration = duration_cast<milliseconds>(endTime - startTime);
        std::cout << "Flight data read in " << duration.count() << "ms" << std::endl;
        return flightData;
    }

} // namespace dfv
