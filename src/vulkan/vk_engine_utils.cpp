#include <fstream>
#include <iostream>
#include <optional>

#include "vk_engine.h"

#include <tiny_obj_loader.h>
#include <vulkan/vk_enum_string_helper.h>

#include "vk_initializers.h"

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

    void VulkanEngine::immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&submitFunc) {
        VkCommandBuffer cmd = uploadContext.commandBuffer;

        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        // Populate the command buffer with the user supplied function
        submitFunc(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        VkSubmitInfo submitInfo = vkinit::submit_info(&cmd);
        VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, uploadContext.uploadFence));

        // Wait for command buffer to finish execution, timeout 10 seconds
        vkWaitForFences(device, 1, &uploadContext.uploadFence, true, 10000000000);
        vkResetFences(device, 1, &uploadContext.uploadFence);

        // Reset the command pool for future reuse
        vkResetCommandPool(device, uploadContext.commandPool, 0);
    }

    std::optional<VkShaderModule> VulkanEngine::loadShaderModule(const std::filesystem::path &filePath) const {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
            return std::nullopt;

        std::streamoff fileSize{file.tellg()};
        // SpirV expects the buffer to be on uint32, reserve an int vector big enough for the entire file
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
        file.close();

        // Create a new shader module, using the buffer we loaded
        VkShaderModuleCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                               .codeSize = buffer.size() * sizeof(uint32_t), // codeSize has to be in bytes
                                               .pCode = buffer.data()};

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            return std::nullopt;

        return {shaderModule};
    }

    AllocatedBuffer VulkanEngine::createUniformBuffer(size_t allocSize, VkBufferUsageFlags usage) const {
        VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                         .pNext = nullptr,
                                         .size = allocSize,
                                         .usage = usage};

        // Allocate UBOs on the GPU while allowing CPU writes (but no reads)
        VmaAllocationCreateInfo vmaAllocInfo = {.usage = VMA_MEMORY_USAGE_CPU_TO_GPU};

        AllocatedBuffer newBuffer = {};
        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation,
                                 nullptr));

        return newBuffer;
    }

    void VulkanEngine::uploadMesh(Mesh &mesh) {
        const size_t bufferSize = mesh.vertices.size() * sizeof(Vertex);
        // Allocate a staging (temporary) buffer
        VkBufferCreateInfo stagingBufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                                .pNext = nullptr,
                                                .size = bufferSize,
                                                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT};

        // Allocate the buffer as writable sequentially from the CPU
        VmaAllocationCreateInfo stagingAllocInfo = {.usage = VMA_MEMORY_USAGE_CPU_ONLY};
        AllocatedBuffer stagingBuffer = {};
        VK_CHECK(vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo,
                                 &stagingBuffer.buffer, &stagingBuffer.allocation, nullptr));

        // Copy vertex data into the staging buffer
        Vertex *stagingVertexData;
        vmaMapMemory(allocator, stagingBuffer.allocation, (void **) &stagingVertexData);
        std::ranges::copy(mesh.vertices, stagingVertexData);
        vmaUnmapMemory(allocator, stagingBuffer.allocation);

        // Allocate the vertex buffer on the GPU
        VkBufferCreateInfo vertexBufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                               .pNext = nullptr,
                                               .size = bufferSize,
                                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};

        VmaAllocationCreateInfo vertexAllocInfo = {.usage = VMA_MEMORY_USAGE_GPU_ONLY};
        VK_CHECK(vmaCreateBuffer(allocator, &vertexBufferInfo, &vertexAllocInfo,
                                 &mesh.vertexBuffer.buffer, &mesh.vertexBuffer.allocation, nullptr));

        // Submit a GPU command to copy the staging buffer to the vertex buffer
        immediateSubmit([&, stagingBuffer](VkCommandBuffer cmd) {
            VkBufferCopy bufferCopy = {.size = bufferSize};
            vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &bufferCopy);
        });

        mainDeletionQueue.pushFunction([=, this]() {
            vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
        });

        // Immediately destroy the staging buffer
        vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
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

    RenderObjectDescriptor VulkanEngine::allocateRenderObject() {
        auto &object = renderObjects.emplace_back();
        return {&object, renderObjects.size() - 1};
    }

    RenderObject *VulkanEngine::getRenderObject(RenderHandle handle) {
        return &renderObjects[handle];
    }

    size_t VulkanEngine::uniformBufferSizeAlignUp(size_t size) const {
        // Calculate required alignment based on minimum device offset alignment
        size_t minUboAlignment = traits.minUniformBufferOffsetAlignment;
        if (minUboAlignment > 0)
            size = (size + minUboAlignment - 1) & ~(minUboAlignment - 1);

        return size;
    }

} // namespace dfv
