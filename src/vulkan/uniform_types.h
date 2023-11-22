#pragma once

#include <glm/glm.hpp>

namespace dfv::uniform {
    /**
     * Uniform buffer object for the scene data.
     */
    struct SceneData {
        glm::vec4 fogColor; // w is for exponent
        glm::vec4 fogDistances; //x for min, y for max, zw unused
        glm::vec4 ambientColor;
        glm::vec4 sunlightDirection; //w for sun power
        glm::vec4 sunlightColor;
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
