#pragma once

#include <functional>

#include <glm/mat4x4.hpp>

#include "vk_mesh.h"
#include <utils/time_types.h>

namespace dfv {

    struct Material {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    };

    struct RenderObject {
        /**
         * @brief Signature of the function that is called every frame to update the object before rendering.
         * @param object The object the function was called on.
         * @param deltaTime The time since the last frame in seconds
         */
        using UpdateFunc = std::function<void(RenderObject &object, seconds_f deltaTime)>;
        UpdateFunc updateFunc; //!< The function to call every frame to update the object before rendering.

        Mesh *mesh;
        Material *material;

        glm::vec3 position;
        glm::vec3 orientation;
        glm::vec3 scale;
        glm::mat4 transformMatrix;

        void computeTransform();
    };

} // namespace dfv
