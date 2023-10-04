#pragma once

#include <vector>
#include <filesystem>

#include <glm/glm.hpp>

#include "vk_types.h"

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

        bool loadFromObj(const std::filesystem::path &filename);
    };

    struct MeshPushConstants {
        glm::vec4 data;
        glm::mat4 renderMatrix;
    };

} // namespace dfv
