#pragma once

#include <vector>

#include "vk_types.h"

namespace dfv {

    class PipelineBuilder {
      public:
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        VkPipelineVertexInputStateCreateInfo vertexInputInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssembly;
        VkViewport viewport;
        VkRect2D scissor;
        VkPipelineRasterizationStateCreateInfo rasterizer;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo multisampling;
        VkPipelineLayout pipelineLayout;

        VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
    };

} // namespace dfv
