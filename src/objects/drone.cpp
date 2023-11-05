#pragma once

#include "drone.h"
#include "map/data_reader.h"

#include <utility>
namespace dfv::objects {
    Drone::Drone(optional<vector<structs::FlightDataPoint>> flightData, dfv::Mesh *mesh, dfv::Material *material,
                 optional<glm::dvec2> bbLowerBounds) {
        this->flightData = std::move(flightData);
        this->bbLowerBounds = bbLowerBounds;
        renderObject = {};
        renderObject.mesh = mesh;
        renderObject.material = material;
        assert(renderObject.mesh != nullptr);
        renderObject.position = {0, 0, 0};
        renderObject.orientation = {0, 0, 0};
        renderObject.scale = {0.01, 0.001, 0.001};
        renderObject.updateFunc = [this](auto &object, seconds_f deltaTime) {
            cout << "drone position: " << object.position.x << ", " << object.position.y << ", " << object.position.z
                 <<", "<< this->currentFlightDataPointIndex << endl;
            if (!this->flightData.has_value()) {
                cout << "Drone: Missing drone flight data" << endl;
                object.computeTransform();
                return;
            }

            auto flightDataV = this->flightData.value();

            if (!this->bbLowerBounds.has_value()) {
                cout << "Drone: Missing bounding box upper bounds" << endl;
                object.computeTransform();
                return;
            }

            auto lowerBounds = this->bbLowerBounds.value();

            auto cords = map::calculateRelativePosition({flightDataV[currentFlightDataPointIndex].lat,
                                                         flightDataV[currentFlightDataPointIndex].lon},
                                                        lowerBounds);


            object.position = {cords.x, flightDataV[currentFlightDataPointIndex].alt/1000, cords.y};
            object.orientation = {flightDataV[currentFlightDataPointIndex].roll,
                                  flightDataV[currentFlightDataPointIndex].pitch,
                                  flightDataV[currentFlightDataPointIndex].yaw};
            object.computeTransform();
            goToNextFlightDataPoint();
        };
    }

    void Drone::goToNextFlightDataPoint() {
        currentFlightDataPointIndex++;
    }

    void Drone::goToPreviousFlightDataPoint() {
        currentFlightDataPointIndex--;
    }

} // namespace dfv::objects
