#pragma once

#include <filesystem>
#include <optional>
#include <vector>

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

        static std::optional<Mesh> loadFromObj(const std::filesystem::path &filename);
    };

} // namespace dfv
