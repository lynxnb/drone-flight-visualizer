#include <iostream>
#include <chrono>
#include <csv.hpp>
#include "data_reader.h"


namespace dfv::map {
    using namespace dfv::structs;

    std::vector<structs::FlightDataPoint> readFlightData(const std::string &csvPath) {
        const auto feetToMeter = 0.3048;
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
                            row["OSD.altitude [ft]"].get<double>() * feetToMeter,
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

    glm::vec2 map::calculateRelativePosition(glm::dvec2 latLon, glm::dvec2 lowerBounds) {
        const auto scale = 100000; //  0.00001 = 1.11 meter

        auto x = (latLon.x - lowerBounds.x) * scale;
        auto y = (latLon.y - lowerBounds.y) * scale;
        return {x, y};
    }
}

