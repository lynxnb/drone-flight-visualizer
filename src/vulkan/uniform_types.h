#pragma once

#include <glm/glm.hpp>

namespace dfv::uniform {
    /**
     * Uniform buffer object for the scene data.
     */
    struct SceneData {
        glm::vec4 ambientColor;
        glm::vec3 sunlightDirection; //w for sun power
        glm::vec4 sunlightColor;
        glm::vec3 eyePos;
        float time;
    };


    /**
     * Uniform buffer object for per-object data.
     */
    struct ObjectData {
        glm::mat4 modelMatrix; // Placeholder, unused
    };

    /**
     * Push constant structure for the model matrix.
     */
    struct MeshPushConstants {
        glm::mat4 modelTransform; //!< Model transform matrix
        glm::mat4 worldTransform; //!< Precomputed model view projection matrix
    };
} // namespace dfv::uniform
