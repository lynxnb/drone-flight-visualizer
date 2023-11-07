#include "drone.h"

#include <utility>
#include <vector>

#include <glm/gtx/transform.hpp>
#include "map/data_reader.h"

namespace dfv::objects {
    Drone::Drone(std::vector<structs::FlightDataPoint> flightData, dfv::Mesh *mesh, dfv::Material *material, std::optional<glm::dvec2> bbLowerBounds)
        : flightData(std::move(flightData)), bbLowerBounds(bbLowerBounds) {
        if (this->flightData.empty()) {
            std::cerr << "Drone: Missing flight data" << std::endl;
        }

        if (!this->bbLowerBounds.has_value()) {
            std::cerr << "Drone: Missing bounding box upper bounds" << std::endl;
        }
    }

    void Drone::update(seconds_f deltaTime, RenderObject *renderObject) {
        auto lowerBounds = this->bbLowerBounds.value();

        auto cords = map::calculateRelativePosition({flightData[currentFlightDataPointIndex].lat,
                                                     flightData[currentFlightDataPointIndex].lon},
                                                    lowerBounds);

        position = {cords.x, flightData[currentFlightDataPointIndex].alt / 1000, cords.y};
        orientation = {flightData[currentFlightDataPointIndex].roll,
                       flightData[currentFlightDataPointIndex].pitch,
                       flightData[currentFlightDataPointIndex].yaw};

        renderObject->transform = glm::translate(glm::mat4{1.0f}, position) *
                                  glm::rotate(glm::mat4{1.0f}, orientation.x, glm::vec3{1.0f, 0.0f, 0.0f}) *
                                  glm::rotate(glm::mat4{1.0f}, orientation.y, glm::vec3{0.0f, 1.0f, 0.0f}) *
                                  glm::scale(glm::mat4{1.0f}, scale);

        goToNextFlightDataPoint();
    }

    void Drone::goToNextFlightDataPoint() {
        currentFlightDataPointIndex++;
    }

    void Drone::goToPreviousFlightDataPoint() {
        currentFlightDataPointIndex--;
    }

} // namespace dfv::objects
