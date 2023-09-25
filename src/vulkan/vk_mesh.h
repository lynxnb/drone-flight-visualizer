#pragma once

#include "vk_types.h"
#include <glm/vec3.hpp>
#include <vector>

namespace dfv {

    struct VertexInputDescription {
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;

        VkPipelineVertexInputStateCreateFlags flags{0};
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;

        static VertexInputDescription getVertexDescription();
    };

    struct Mesh {
        std::vector<Vertex> vertices;

        AllocatedBuffer vertexBuffer;
    };

} // namespace dfv
