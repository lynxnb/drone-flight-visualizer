#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

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
        glm::vec2 uv;

        static VertexInputDescription getVertexDescription();
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        AllocatedBuffer vertexBuffer;
        AllocatedBuffer indexBuffer;

        static std::optional<Mesh> loadFromObj(const std::filesystem::path &filename);
    };

} // namespace dfv
