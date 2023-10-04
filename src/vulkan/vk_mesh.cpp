#include "vk_mesh.h"

#include <iostream>

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

        // Color will be stored at Location 2
        VkVertexInputAttributeDescription colorAttribute = {};
        colorAttribute.binding = 0;
        colorAttribute.location = 2;
        colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        colorAttribute.offset = offsetof(Vertex, color);

        description.attributes.push_back(positionAttribute);
        description.attributes.push_back(normalAttribute);
        description.attributes.push_back(colorAttribute);
        return description;
    }

    bool Mesh::loadFromObj(const std::filesystem::path &filename) {
        // Attrib will contain the vertex arrays of the file
        tinyobj::attrib_t attrib;
        // Shapes contains the info for each separate object in the file
        std::vector<tinyobj::shape_t> shapes;
        // Materials contains the information about the material of each shape, but we won't use it
        std::vector<tinyobj::material_t> materials;

        // Error and warning output from the load function
        std::string err;

        auto objDir = filename.parent_path().string() + "/";
        // Load the OBJ file
        bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &err,
                                       filename.string().c_str(),
                                       objDir.c_str());

        // Error handling
        if (!err.empty())
            std::cerr << err << std::endl;

        if (!loaded)
            return false;

        // Loop over shapes
        for (auto &shape : shapes) {
            // Loop over faces
            size_t indexOffset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                // Hardcode loading triangles only
                constexpr int fv = 3;

                // Loop over vertices in the face
                for (size_t v = 0; v < fv; v++) {
                    // Access to vertex
                    tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];

                    // Vertex position
                    tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                    tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                    tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                    // Vertex normal
                    tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                    tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                    tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                    // Copy it into our vertex
                    Vertex newVert{
                            .position = {vx, vy, vz},
                            .normal = {nx, ny, nz},
                            // Set the vertex color as the vertex normal, this is just for display purposes
                            .color = newVert.normal,
                    };

                    vertices.push_back(newVert);
                }
                indexOffset += fv;
            }
        }

        return true;
    }

} // namespace dfv
