#include "vk_mesh.h"

#include <iostream>
#include <numeric>

#include <tiny_obj_loader.h>

namespace dfv {

    VertexInputDescription Vertex::getVertexDescription() {
        VertexInputDescription description;

        // We will have just 1 vertex buffer binding, with a per-vertex rate
        VkVertexInputBindingDescription mainBinding = {};
        mainBinding.binding = 0;
        mainBinding.stride = sizeof(Vertex);
        mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        description.bindings.push_back(mainBinding);

        // Position will be stored at Location 0
        VkVertexInputAttributeDescription positionAttribute = {};
        positionAttribute.binding = 0;
        positionAttribute.location = 0;
        positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        positionAttribute.offset = offsetof(Vertex, position);

        // Normal will be stored at Location 1
        VkVertexInputAttributeDescription normalAttribute = {};
        normalAttribute.binding = 0;
        normalAttribute.location = 1;
        normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        normalAttribute.offset = offsetof(Vertex, normal);

        // UV will be stored at Location 2
        VkVertexInputAttributeDescription uvAttribute = {};
        uvAttribute.binding = 0;
        uvAttribute.location = 2;
        uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
        uvAttribute.offset = offsetof(Vertex, uv);

        description.attributes.push_back(positionAttribute);
        description.attributes.push_back(normalAttribute);
        description.attributes.push_back(uvAttribute);
        return description;
    }

    std::optional<Mesh> Mesh::loadFromObj(const std::filesystem::path &filename) {
        // Attrib will contain the vertex arrays of the file
        tinyobj::attrib_t attrib;
        // Shapes contains the info for each separate object in the file
        std::vector<tinyobj::shape_t> shapes;
        // Materials contains the information about the material of each shape, but we won't use it
        std::vector<tinyobj::material_t> materials;

        // Error and warning output from the load function
        std::string err, warn;

        auto objDir = filename.parent_path().string() + "/";
        // Load the OBJ file
        bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &err,
                                       filename.string().c_str(),
                                       objDir.c_str());

        // Error handling
        if (!err.empty())
            std::cerr << err << std::endl;

        if (!loaded)
            return std::nullopt;

        std::vector<Vertex> vertices;
        vertices.reserve(attrib.vertices.size() / 3);
        // Get total index count by summing up the number of indices in each shape
        size_t indexCount = std::accumulate(shapes.begin(), shapes.end(), 0,
                                            [](size_t acc, const tinyobj::shape_t &shape) {
                                                return acc + shape.mesh.indices.size();
                                            });
        std::vector<uint32_t> indices;
        indices.reserve(indexCount);

        for (const auto &shape : shapes) {
            for (const auto &index : shape.mesh.indices) {
                Vertex vertex{};
                vertex.position = {attrib.vertices[3 * index.vertex_index + 0],
                                   attrib.vertices[3 * index.vertex_index + 1],
                                   attrib.vertices[3 * index.vertex_index + 2]};

                vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                                 attrib.normals[3 * index.normal_index + 1],
                                 attrib.normals[3 * index.normal_index + 2]};

                if (index.texcoord_index >= 0)
                    vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                                 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

                vertices.push_back(vertex);
                indices.push_back(indices.size());
            }
        }

        return Mesh{.vertices = std::move(vertices),
                    .indices = std::move(indices)};
    }

} // namespace dfv
