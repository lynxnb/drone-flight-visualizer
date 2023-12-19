#pragma once

#include <glm/mat4x4.hpp>

#include "vk_mesh.h"

namespace dfv {

    struct Material {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    };

    using RenderHandle = size_t; //!< The handle type for render objects
    constexpr RenderHandle NullHandle = -1; //!< The null handle value

    struct RenderObject {
        Mesh *mesh{nullptr};
        Material *material{nullptr};

        glm::mat4 transform{};
    };

    struct RenderObjectDescriptor {
        RenderObject *object{nullptr};
        RenderHandle handle{NullHandle};
    };

} // namespace dfv
