#include "drone.h"

#include <utility>
#include <vector>

#include <glm/gtx/transform.hpp>
#include "map/data_reader.h"

namespace dfv::objects {
    Drone::Drone(FlightData &flightData)
        : flightData(flightData){

    }

    void Drone::update(seconds_f deltaTime, RenderObject *renderObject) {
        auto cords = flightData.getPoint(currentTimeStamp);

        position = {cords.x, cords.y, cords.z};
        orientation = {cords.pitch, cords.roll, cords.yaw};

        renderObject->transform = glm::translate(glm::mat4{1.0f}, position) *
                                  glm::rotate(glm::mat4{1.0f}, orientation.x, glm::vec3{1.0f, 0.0f, 0.0f}) *
                                  glm::rotate(glm::mat4{1.0f}, orientation.y, glm::vec3{0.0f, 1.0f, 0.0f}) *
                                  glm::rotate(glm::mat4{1.0f}, orientation.z, glm::vec3{0.0f, 0.0f, 1.0f}) *
                                  glm::scale(glm::mat4{1.0f}, scale);

        // if ( button forward)
        currentTimeStamp += deltaTime;
        // if ( button backward)
        // currentTimeStamp -= deltaTime;
    }

} // namespace dfv::objects
