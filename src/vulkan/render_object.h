#pragma once

#include <glm/mat4x4.hpp>

#include "vk_mesh.h"

namespace dfv {

    struct Material {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    };

    struct RenderObject {
        Mesh *mesh;
        Material *material;

        glm::mat4 transformMatrix;
    };

} // namespace dfv
