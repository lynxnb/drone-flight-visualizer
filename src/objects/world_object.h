#pragma once

#include <glm/vec3.hpp>

class WorldObject {
public:
    virtual ~WorldObject();

    virtual glm::vec3 getWorldPosition() = 0;
};
