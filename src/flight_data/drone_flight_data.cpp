#include "drone_flight_data.h"

#include <glm/glm.hpp>

#include <csv.hpp>
#include <utility>
#include <utils/time_types.h>
#define M_PI 3.14159265358979323846f

namespace dfv {
    DroneFlightData::DroneFlightData(std::filesystem::path path)
        : path(std::move(path)), boundingBox() {}

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

    std::vector<FlightDataPoint> &DroneFlightData::getPath() {
        return flightDataPoints;
    }

    FlightDataPoint DroneFlightData::getPoint(seconds_f timestamp) {
        // upper_bound returns the first element that is greater than the timestamp, aka the next point
        // if the timestamp is 0 or less, this will return the first point
        auto nextPointIt = std::upper_bound(flightDataPoints.begin(), flightDataPoints.end(), timestamp,
                                            [](const seconds_f &timestamp, const FlightDataPoint &point) {
                                                return timestamp.count() < point.timestamp;
                                            });

        // Get the current point by going back one element
        auto pointIt = nextPointIt != flightDataPoints.begin() ? nextPointIt - 1 : nextPointIt;

        if (nextPointIt == flightDataPoints.end())
            return *pointIt;

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
                .yaw = lerpAngle(pointIt->yaw, nextPointIt->yaw),
                .pitch = lerpAngle(pointIt->pitch, nextPointIt->pitch),
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
        return boundingBox;
    }

    std::vector<FlightDataPoint> DroneFlightData::loadFlightData(const std::string &csvPath) {
        const double feetToMeter = 0.3048;
        using namespace csv;

        const auto startTime = clock::now();

        using namespace csv;
        CSVReader reader(csvPath);
        std::vector<FlightDataPoint> flightData;

        for (CSVRow &row : reader) {
            const Coordinate coords = {.lat = row["OSD.latitude"].get<float>(),
                                       .lon = row["OSD.longitude"].get<float>(),
                                       .alt = row["OSD.altitude [ft]"].get<double>() * feetToMeter};

            // set initial position if not set yet
            if (!initialPosition) {
                initialPosition = coords;
                boundingBox.llLon = coords.lon;
                boundingBox.llLat = coords.lat;
                boundingBox.urLon = coords.lon;
                boundingBox.urLat = coords.lat;
            }

            boundingBox.llLat = std::min(boundingBox.llLat, coords.lat);
            boundingBox.llLon = std::min(boundingBox.llLon, coords.lon);
            boundingBox.urLat = std::max(boundingBox.urLat, coords.lat);
            boundingBox.urLon = std::max(boundingBox.urLon, coords.lon);

            // calculate position in relation to initial position
            Coordinate relativeCoords = calculateRelativePosition(coords, *initialPosition);

            flightData.emplace_back(
                    row["OSD.flyTime [s]"].get<float>(),
                    relativeCoords.lon,
                    relativeCoords.alt,
                    relativeCoords.lat,
                    glm::radians(row["OSD.yaw"].get<float>()),
                    glm::radians(row["OSD.pitch"].get<float>()),
                    glm::radians(row["OSD.roll"].get<float>()));
        }

        const auto endTime = clock::now();
        const auto duration = duration_cast<milliseconds>(endTime - startTime);
        std::cout << "Flight data read in " << duration.count() << "ms" << std::endl;

        return flightData;
    }

} // namespace dfv
