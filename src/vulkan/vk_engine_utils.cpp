#include <fstream>
#include <iostream>
#include <optional>

#include "vk_engine.h"

#include <vulkan/vk_enum_string_helper.h>

#include "vk_initializers.h"
#include <utils/stb_image_loader.h>

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

    void VulkanEngine::immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&submitFunc) const {
        VkCommandBuffer cmd = uploadContext.commandBuffer;

        const VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        // Populate the command buffer with the user supplied function
        submitFunc(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        const VkSubmitInfo submitInfo = vkinit::submit_info(&cmd);
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

        const std::streamoff fileSize{file.tellg()};
        // SpirV expects the buffer to be on uint32, reserve an int vector big enough for the entire file
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
        file.close();

        // Create a new shader module, using the buffer we loaded
        const VkShaderModuleCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                                     .codeSize = buffer.size() * sizeof(uint32_t), // codeSize has to be in bytes
                                                     .pCode = buffer.data()};

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            return std::nullopt;

        return {shaderModule};
    }

    AllocatedBuffer VulkanEngine::createUniformBuffer(const size_t allocSize, const VkBufferUsageFlags usage) const {
        const VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                               .pNext = nullptr,
                                               .size = allocSize,
                                               .usage = usage};

        // Allocate UBOs on the GPU while allowing CPU writes (but no reads)
        const VmaAllocationCreateInfo vmaAllocInfo = {.usage = VMA_MEMORY_USAGE_CPU_TO_GPU};

        AllocatedBuffer newBuffer = {};
        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr));
        return newBuffer;
    }

    AllocatedBuffer VulkanEngine::createStagingBuffer(const size_t allocSize) const {
        const VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                               .pNext = nullptr,
                                               .size = allocSize,
                                               .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT};

        // Allocate staging buffer on the CPU
        const VmaAllocationCreateInfo vmaAllocInfo = {.usage = VMA_MEMORY_USAGE_CPU_ONLY};

        AllocatedBuffer newBuffer = {};
        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.buffer, &newBuffer.allocation, nullptr));
        return newBuffer;
    }

    void VulkanEngine::uploadMesh(Mesh &mesh) {
        const size_t vertexBufSize = mesh.vertices.size() * sizeof(Vertex);
        const size_t indexBufSize = mesh.indices.size() * sizeof(uint32_t);

        const size_t stagingBufferSize = vertexBufSize + indexBufSize;

        // Allocate staging buffer for vertex + index data
        const AllocatedBuffer stagingBuffer = createStagingBuffer(stagingBufferSize);

        Vertex *stagingVertexData;
        vmaMapMemory(allocator, stagingBuffer.allocation, reinterpret_cast<void **>(&stagingVertexData));
        // Copy vertex data into the staging buffer
        std::ranges::copy(mesh.vertices, stagingVertexData);

        // Copy index data past the vertex data
        const auto stagingIndexData = reinterpret_cast<uint32_t *>(stagingVertexData + mesh.vertices.size());
        std::ranges::copy(mesh.indices, stagingIndexData);
        vmaUnmapMemory(allocator, stagingBuffer.allocation);

        // Allocate the vertex and index buffers on the GPU
        const VkBufferCreateInfo vertexBufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                                     .pNext = nullptr,
                                                     .size = vertexBufSize,
                                                     .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};
        const VkBufferCreateInfo indexBufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                                    .pNext = nullptr,
                                                    .size = indexBufSize,
                                                    .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};

        const VmaAllocationCreateInfo allocInfo = {.usage = VMA_MEMORY_USAGE_GPU_ONLY};
        VK_CHECK(vmaCreateBuffer(allocator, &vertexBufferInfo, &allocInfo,
                                 &mesh.vertexBuffer.buffer, &mesh.vertexBuffer.allocation, nullptr));
        VK_CHECK(vmaCreateBuffer(allocator, &indexBufferInfo, &allocInfo,
                                 &mesh.indexBuffer.buffer, &mesh.indexBuffer.allocation, nullptr));

        // Submit a GPU command to copy the staging buffer to the vertex and index buffers
        immediateSubmit([&](VkCommandBuffer cmd) {
            const VkBufferCopy vertexCopy = {.srcOffset = 0,
                                             .dstOffset = 0,
                                             .size = vertexBufSize};
            const VkBufferCopy indexCopy = {.srcOffset = vertexBufSize,
                                            .dstOffset = 0,
                                            .size = indexBufSize};
            vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &vertexCopy);
            vkCmdCopyBuffer(cmd, stagingBuffer.buffer, mesh.indexBuffer.buffer, 1, &indexCopy);
        });

        mainDeletionQueue.pushFunction([=, this] {
            vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
            vmaDestroyBuffer(allocator, mesh.indexBuffer.buffer, mesh.indexBuffer.allocation);
        });

        // Immediately destroy the staging buffer
        vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
    }

    void VulkanEngine::uploadTexture(Texture &texture, std::span<std::byte> data) {
        // Allocate a staging buffer for texture data
        const AllocatedBuffer stagingBuffer = createStagingBuffer(data.size_bytes());

        // Copy data to the staging buffer
        std::byte *dest;
        vmaMapMemory(allocator, stagingBuffer.allocation, reinterpret_cast<void **>(&dest));
        std::ranges::copy(data, dest);
        vmaUnmapMemory(allocator, stagingBuffer.allocation);

        const VkImageCreateInfo imageInfo = vkinit::image_create_info(texture.format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, texture.extent);

        // Allocate the image on the GPU
        const VmaAllocationCreateInfo imageAllocInfo = {.usage = VMA_MEMORY_USAGE_GPU_ONLY};

        // Allocate and create the image
        VK_CHECK(vmaCreateImage(allocator, &imageInfo, &imageAllocInfo, &texture.image.image, &texture.image.allocation, nullptr));

        immediateSubmit([&](VkCommandBuffer cmd) {
            const VkImageSubresourceRange range = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                   .baseMipLevel = 0,
                                                   .levelCount = 1,
                                                   .baseArrayLayer = 0,
                                                   .layerCount = 1};

            const VkImageMemoryBarrier imageBarrierToTransferOptimal = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                                        .srcAccessMask = 0,
                                                                        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                                                                        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                                        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                                        .image = texture.image.image,
                                                                        .subresourceRange = range};

            // Use a pipeline barrier to transition the image layout to optimal as transfer destination
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &imageBarrierToTransferOptimal);

            const VkBufferImageCopy copyRegion = {
                    .bufferOffset = 0,
                    .bufferRowLength = 0,
                    .bufferImageHeight = 0,
                    .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                         .mipLevel = 0,
                                         .baseArrayLayer = 0,
                                         .layerCount = 1},
                    .imageExtent = texture.extent
            };

            // Perform the copy from staging buffer to destination image
            vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, texture.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            VkImageMemoryBarrier imageBarrierToShaderOptimal = imageBarrierToTransferOptimal;
            imageBarrierToShaderOptimal.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrierToShaderOptimal.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageBarrierToShaderOptimal.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageBarrierToShaderOptimal.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            // Transition the image layout to optimal for shader reads
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &imageBarrierToShaderOptimal);
        });

        const VkImageViewCreateInfo viewInfo = vkinit::imageview_create_info(texture.format, texture.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &texture.imageView));

        mainDeletionQueue.pushFunction([=, this] {
            vkDestroyImageView(device, texture.imageView, nullptr);
            vmaDestroyImage(allocator, texture.image.image, texture.image.allocation);
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
        const auto it = materials.find(name);
        if (it == materials.end())
            return nullptr;

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

    Mesh *VulkanEngine::insertMesh(const std::string &name, Mesh &&mesh) {
        auto [meshIt, isNewInsertion] = meshes.insert_or_assign(name, std::move(mesh));
        if (!isNewInsertion)
            std::cerr << "Warning: mesh '" << name << "' was overwritten" << std::endl;

        uploadMesh(meshIt->second);
        return &meshIt->second;
    }

    Mesh *VulkanEngine::getMesh(const std::string &name) {
        const auto it = meshes.find(name);
        if (it == meshes.end())
            return nullptr;

        return &it->second;
    }

    Texture *VulkanEngine::createTexture(const std::string &name, const std::filesystem::path &filename) {
        const StbImageLoader loader{filename};

        Texture texture = {
                .extent = {.width = loader.width(),
                           .height = loader.height(),
                           .depth = 1},
                .format = VK_FORMAT_R8G8B8A8_SRGB,
        };
        uploadTexture(texture, loader.data());

        auto [textureIt, isNewInsertion] = textures.insert_or_assign(name, texture);
        if (!isNewInsertion)
            std::cerr << "Warning: texture '" << name << "' was overwritten" << std::endl;

        return &textureIt->second;
    }

    Texture *VulkanEngine::insertTexture(const std::string &name, Texture &&texture, const std::span<std::byte> data) {
        uploadTexture(texture, data);

        auto [textureIt, isNewInsertion] = textures.insert_or_assign(name, texture);
        if (!isNewInsertion)
            std::cerr << "Warning: texture '" << name << "' was overwritten" << std::endl;

        return &textureIt->second;
    }

    Texture *VulkanEngine::getTexture(const std::string &name) {
        const auto it = textures.find(name);
        if (it == textures.end())
            return nullptr;

        return &it->second;
    }

    RenderObjectDescriptor VulkanEngine::allocateRenderObject() {
        auto &object = renderObjects.emplace_back();
        return {&object, renderObjects.size() - 1};
    }

    void VulkanEngine::applyTexture(const RenderHandle handle, const Texture *texture) {
        if (!texture) {
            std::cerr << "Warning: trying to apply a null texture to a render object" << std::endl;
            return;
        }

        const auto &renderObj = *getRenderObject(handle);

        const VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);
        VkSampler blockySampler;
        vkCreateSampler(device, &samplerInfo, nullptr, &blockySampler);

        mainDeletionQueue.pushFunction([=, this] {
            vkDestroySampler(device, blockySampler, nullptr);
        });

        // Allocate the descriptor set for the material set
        const VkDescriptorSetAllocateInfo allocInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                       .descriptorPool = descriptorPool,
                                                       .descriptorSetCount = 1,
                                                       .pSetLayouts = &materialSetLayout};

        vkAllocateDescriptorSets(device, &allocInfo, &renderObj.material->textureSet);

        // Write to the descriptor set so that it points to the given texture
        VkDescriptorImageInfo imageBufferInfo = {.sampler = blockySampler,
                                                 .imageView = texture->imageView,
                                                 .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        const VkWriteDescriptorSet textureSetWrite = vkinit::write_descriptor_image(
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, renderObj.material->textureSet, &imageBufferInfo, 0);

        vkUpdateDescriptorSets(device, 1, &textureSetWrite, 0, nullptr);
    }

    RenderObject *VulkanEngine::getRenderObject(const RenderHandle handle) {
        return &renderObjects[handle];
    }

    size_t VulkanEngine::uniformBufferSizeAlignUp(size_t size) const {
        // Calculate required alignment based on minimum device offset alignment
        const size_t minUboAlignment = traits.minUniformBufferOffsetAlignment;
        if (minUboAlignment > 0)
            size = (size + minUboAlignment - 1) & ~(minUboAlignment - 1);

        return size;
    }

} // namespace dfv
