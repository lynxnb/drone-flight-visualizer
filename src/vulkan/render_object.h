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

    using RenderHandle = size_t; //!< The handle type for render objects

    struct RenderObject {
        Mesh *mesh{nullptr};
        Material *material{nullptr};

        glm::mat4 transform{};
    };

    struct RenderObjectDescriptor {
        RenderObject *object{nullptr};
        RenderHandle handle{};
    };

} // namespace dfv
