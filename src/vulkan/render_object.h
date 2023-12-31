#pragma once

#include <glm/mat4x4.hpp>

#include "vk_mesh.h"

namespace dfv {

    struct Material {
        VkDescriptorSet textureSet{VK_NULL_HANDLE};
        VkPipeline pipeline{VK_NULL_HANDLE};
        VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
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
