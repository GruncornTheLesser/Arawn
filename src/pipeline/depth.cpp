#define ARAWN_INCLUDE_VULKAN
#include <pipeline/depth.h>
#include <core/device.h>
#include <attachment/depth.h>
#include <attachment/ztree.h>
#include <primitive/vertex.h>


using namespace arawn::pipeline;


alignas(uint32_t) const unsigned char vertSpv[] = {
#embed "shaders/depth.vert.spv"
};

alignas(uint32_t) const unsigned char fragSpv[] = {
#embed "shaders/depth.frag.spv"
};

void DepthOnly::create(const core::Device &device, const attachment::Depth& depth) {
    auto sets = std::to_array<VkDescriptorSetLayout>({ nullptr });// TODO:

    VkPushConstantRange range{ 
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushData)
    };
    VkPipelineLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = static_cast<uint32_t>(sets.size()),
        .pSetLayouts = sets.data(),
        .pushConstantRangeCount = 0, // 1u,
        .pPushConstantRanges = &range,
    };
    VK_ASSERT(vkCreatePipelineLayout(device.device, &info, nullptr, &layout));
    
    VkShaderModule vert, frag; 
    VkShaderModuleCreateInfo vertInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = sizeof(vertSpv),
        .pCode = reinterpret_cast<const uint32_t*>(vertSpv),
    };
    VK_ASSERT(vkCreateShaderModule(device.device, &vertInfo, nullptr, &vert));

    VkShaderModuleCreateInfo fragInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = sizeof(fragSpv),
        .pCode = reinterpret_cast<const uint32_t*>(fragSpv),
    };
    VK_ASSERT(vkCreateShaderModule(device.device, &fragInfo, nullptr, &frag));
    
    auto stages = std::to_array<VkPipelineShaderStageCreateInfo>({ 
        { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert,
            .pName = "main"
        },
        { 
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag,
            .pName = "main"
        }
    });

    auto vertexBindings = std::to_array<VkVertexInputBindingDescription>({ 
        {
            .binding = 0,
            .stride = sizeof(primitive::Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        }
    });

    auto vertexAttributes = std::to_array<VkVertexInputAttributeDescription>({
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT, // TODO: double check this
            .offset = offsetof(primitive::Vertex, position),
        }
    });

    VkPipelineVertexInputStateCreateInfo vertexInput{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size()),
        .pVertexBindingDescriptions = vertexBindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
        .pVertexAttributeDescriptions = vertexAttributes.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };
    
    // viewport/scissor are dynamic, only the counts matter
    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterization{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT, // TODO: depends on settings
        .sampleShadingEnable = VK_FALSE,
    };
    
    VkPipelineColorBlendStateCreateInfo blendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 0,
        .pAttachments = nullptr,
    };

    auto dynamics = std::to_array<VkDynamicState>({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamics.size()),
        .pDynamicStates = dynamics.data(),
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    // dynamic rendering (Vulkan 1.3): no VkRenderPass
    VkPipelineRenderingCreateInfo rendering{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .viewMask = 0,
        .colorAttachmentCount = 0,
        .pColorAttachmentFormats = nullptr,
        .depthAttachmentFormat = depth.format,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };
    
    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering,
        .flags = 0,
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisample,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &blendState,
        .pDynamicState = &dynamicState,
        .layout = layout,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };
    VK_ASSERT(vkCreateGraphicsPipelines(device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

    vkDestroyShaderModule(device.device, vert, nullptr);
    vkDestroyShaderModule(device.device, frag, nullptr);

}

void DepthOnly::destroy(const core::Device &device) {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, layout, nullptr);
}