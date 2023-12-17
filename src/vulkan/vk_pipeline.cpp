#include "vk_pipeline.h"

#include <iostream>

namespace dfv {

    VkPipeline PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass) const {
        // Dynamic state declaration
        const std::vector dynamicState = {VK_DYNAMIC_STATE_VIEWPORT,
                                          VK_DYNAMIC_STATE_SCISSOR};

        // No static viewports or scissors
        const VkPipelineViewportStateCreateInfo viewportState = {.viewportCount = 0,
                                                                 .scissorCount = 0};

        // Set the state as being dynamic
        const VkPipelineDynamicStateCreateInfo dynamicStateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                                   .dynamicStateCount = static_cast<uint32_t>(dynamicState.size()),
                                                                   .pDynamicStates = dynamicState.data()};

        // Setup dummy color blending. We aren't using transparent objects yet
        // The blending is just "no blend", but we do write to the color attachment
        VkPipelineColorBlendStateCreateInfo colorBlending = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                                             .pNext = nullptr,
                                                             .logicOpEnable = VK_FALSE,
                                                             .logicOp = VK_LOGIC_OP_COPY,
                                                             .attachmentCount = 1,
                                                             .pAttachments = &colorBlendAttachment};

        // Build the pipeline
        const VkGraphicsPipelineCreateInfo pipelineInfo = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                                           .pNext = nullptr,
                                                           .stageCount = static_cast<uint32_t>(shaderStages.size()),
                                                           .pStages = shaderStages.data(),
                                                           .pVertexInputState = &vertexInputInfo,
                                                           .pInputAssemblyState = &inputAssembly,
                                                           .pViewportState = &viewportState,
                                                           .pRasterizationState = &rasterizer,
                                                           .pMultisampleState = &multisampling,
                                                           .pDepthStencilState = &depthStencil,
                                                           .pColorBlendState = &colorBlending,
                                                           .pDynamicState = &dynamicStateInfo,
                                                           .layout = pipelineLayout,
                                                           .renderPass = pass,
                                                           .subpass = 0,
                                                           .basePipelineHandle = VK_NULL_HANDLE};

        VkPipeline newPipeline;
        if (vkCreateGraphicsPipelines(
                    device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
            std::cout << "Failed to create pipeline\n";
            return VK_NULL_HANDLE;
        } else {
            return newPipeline;
        }
    }

} // namespace dfv
