#include "mock_flight_data.h"

#include <utility>

#define M_PI 3.14159265358979323846f

namespace dfv {
    static constexpr std::array dataset = {
            FlightDataPoint{0.0,      2.0, 0.0,      0.0,      0.0, 0.0, 0.0},
            FlightDataPoint{1.0,  1.41421, 0.0,  1.41421, 0.628319, 0.0, 0.0},
            FlightDataPoint{2.0,      0.0, 0.0,      2.0,  1.25664, 0.0, 0.0},
            FlightDataPoint{3.0, -1.41421, 0.0,  1.41421,  1.88496, 0.0, 0.0},
            FlightDataPoint{4.0,     -2.0, 0.0,      0.0,  2.51327, 0.0, 0.0},
            FlightDataPoint{5.0, -1.41421, 0.0, -1.41421,  3.14159, 0.0, 0.0},
            FlightDataPoint{6.0,      0.0, 0.0,     -2.0,  3.76991, 0.0, 0.0},
            FlightDataPoint{7.0,  1.41421, 0.0, -1.41421,  4.39822, 0.0, 0.0},
            FlightDataPoint{8.0,      2.0, 0.0,      0.0,  5.02654, 0.0, 0.0},
    };

    static constexpr FlightBoundingBox boundingBox = [] {
        FlightBoundingBox box{};
        for (const auto &point : dataset) {
            box.llLat = std::min(box.llLat, static_cast<double>(point.x));
            box.llLon = std::min(box.llLon, static_cast<double>(point.y));
            box.urLat = std::max(box.urLat, static_cast<double>(point.x));
            box.urLon = std::max(box.urLon, static_cast<double>(point.y));
        }

        return box;
    }();

    bool MockFlightData::load() {
        return true;
    }

    Coordinate MockFlightData::getInitialPosition() {
        return {.lat = 45.5009309, .lon = 9.1553888, .alt = 120.0};
    }

    FlightDataPoint MockFlightData::getPoint(seconds_f timestamp) {
        // Use the modulo of the timestamp to wrap around the data set
        float time = std::fmod(timestamp.count(), float{dataset.size()});

        // Find the data point after the one we want
        auto nextPointIt = std::upper_bound(dataset.begin(), dataset.end(), time, [](float value, const FlightDataPoint &point) {
            return value < point.timestamp;
        });

        // Find the current data point
        auto pointIt = [&] {
            if (nextPointIt != dataset.begin())
                return std::prev(nextPointIt);
            else
                return dataset.end() - 1; // Wrap around if we're at the first data point
        }();

        // Wrap around if we're at the last data point
        if (nextPointIt == dataset.end())
            nextPointIt = dataset.begin();

        // Interpolate between the current point and the next one
        float lerpTime = time - pointIt->timestamp;

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

        return {.timestamp = pointIt->timestamp,
                .x = lerp(pointIt->x, nextPointIt->x),
                .y = lerp(pointIt->y, nextPointIt->y),
                .z = lerp(pointIt->z, nextPointIt->z),
                .yaw = lerpAngle(pointIt->yaw, nextPointIt->yaw),
                .pitch = lerpAngle(pointIt->pitch, nextPointIt->pitch),
                .roll = lerpAngle(pointIt->roll, nextPointIt->roll)};
    }

    seconds_f MockFlightData::getDuration() {
        return seconds_f{dataset.back().timestamp - dataset.front().timestamp};
    }

    seconds_f MockFlightData::getStartTime() {
        return seconds_f{dataset.front().timestamp};
    }

    seconds_f MockFlightData::getEndTime() {
        return seconds_f{dataset.back().timestamp};
    }

    FlightBoundingBox MockFlightData::getBoundingBox() {
        return boundingBox;
    }

    std::vector<FlightDataPoint> &MockFlightData::getPath() {
        static std::vector<FlightDataPoint> path{dataset.begin(), dataset.end()};
        return path;
    }
} // namespace dfv
