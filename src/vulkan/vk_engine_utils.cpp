#include <fstream>
#include <iostream>

#include "vk_engine.h"

#include <tiny_obj_loader.h>
#include <vulkan/vk_enum_string_helper.h>

#define SOURCE_LOCATION __builtin_FILE() << ":" << __builtin_LINE() << " (" << __builtin_FUNCTION() << ")"

#define VK_CHECK(x)                                                                                           \
    do {                                                                                                      \
        VkResult err = x;                                                                                     \
        if (err) {                                                                                            \
            std::cerr << SOURCE_LOCATION << ": Detected Vulkan error: " << string_VkResult(err) << std::endl; \
            std::abort();                                                                                     \
        }                                                                                                     \
    } while (0)

namespace dfv {

    bool VulkanEngine::loadShaderModule(const std::filesystem::path &filePath, VkShaderModule *outShaderModule) const {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
            return false;

        std::streamoff fileSize{file.tellg()};
        // Spirv expects the buffer to be on uint32, reserve an int vector big enough for the entire file
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char *>(buffer.data()), fileSize);

        file.close();

        // Create a new shader module, using the buffer we loaded
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        // codeSize has to be in bytes
        createInfo.codeSize = buffer.size() * sizeof(uint32_t);
        createInfo.pCode = buffer.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            return false;

        *outShaderModule = shaderModule;
        return true;
    }

    AllocatedBuffer VulkanEngine::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) const {
        VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                         .pNext = nullptr,
                                         .size = allocSize,
                                         .usage = usage};

        VmaAllocationCreateInfo vmaAllocInfo = {.usage = memoryUsage};

        AllocatedBuffer newBuffer = {};
        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation,
                                 nullptr));

        return newBuffer;
    }

    void VulkanEngine::uploadMesh(Mesh &mesh) {
        // Allocate vertex buffer
        mesh.vertexBuffer = createBuffer(mesh.vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                         VMA_MEMORY_USAGE_CPU_TO_GPU);

        // Add the destruction of this buffer to the deletion queue
        mainDeletionQueue.pushFunction([=, this]() {
            vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
        });

        // Copy vertex data
        Vertex *vertexData;
        vmaMapMemory(allocator, mesh.vertexBuffer.allocation, (void **) &vertexData);
        std::ranges::copy(mesh.vertices, vertexData);
        vmaUnmapMemory(allocator, mesh.vertexBuffer.allocation);
    }

    Material *VulkanEngine::createMaterial(const std::string &name, VkPipeline pipeline, VkPipelineLayout layout) {
        Material material = {};
        material.pipeline = pipeline;
        material.pipelineLayout = layout;

        auto [materialIt, isNewInsertion] = materials.insert_or_assign(name, material);
        if (!isNewInsertion)
            std::cerr << "Warning: material '" << name << "' was overwritten" << std::endl;

        return &materialIt->second;
    }

    Material *VulkanEngine::getMaterial(const std::string &name) {
        auto it = materials.find(name);
        if (it == materials.end())
            return nullptr;
        else
            return &it->second;
    }

    Mesh *VulkanEngine::createMesh(const std::string &name, const std::filesystem::path &filename) {
        auto meshOpt = Mesh::loadFromObj(filename);
        if (!meshOpt.has_value())
            return nullptr;

        auto [meshIt, isNewInsertion] = meshes.insert_or_assign(name, std::move(meshOpt.value()));
        if (!isNewInsertion)
            std::cerr << "Warning: mesh '" << name << "' was overwritten" << std::endl;

        uploadMesh(meshIt->second);
        return &meshIt->second;
    }

    Mesh *VulkanEngine::getMesh(const std::string &name) {
        auto it = meshes.find(name);
        if (it == meshes.end())
            return nullptr;
        else
            return &(*it).second;
    }

    size_t VulkanEngine::uniformBufferSizeAlignUp(size_t size) const {
        // Calculate required alignment based on minimum device offset alignment
        size_t minUboAlignment = traits.minUniformBufferOffsetAlignment;
        if (minUboAlignment > 0)
            size = (size + minUboAlignment - 1) & ~(minUboAlignment - 1);

        return size;
    }

} // namespace dfv
