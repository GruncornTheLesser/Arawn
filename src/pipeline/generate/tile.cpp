#define ARAWN_INCLUDE_VULKAN
#include <pipeline/generate/tile.h>
#include <core/device.h>

using namespace arawn;

alignas(uint32_t) const unsigned char compSpv[] = {
#embed "shaders/generate/tiles.comp.spv"
};

void pipeline::gen::Tile::create(const core::Device &device) {
    VkPushConstantRange range{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PushData)
    };

    auto sets = std::to_array<VkDescriptorSetLayout>({ 
        nullptr
    });

    VkPipelineLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = static_cast<uint32_t>(sets.size()),
        .pSetLayouts = sets.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range,
    };
    VK_ASSERT(vkCreatePipelineLayout(device, &info, nullptr, &layout));

    VkShaderModule comp;
    VkShaderModuleCreateInfo compInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = sizeof(compSpv),
        .pCode = reinterpret_cast<const uint32_t*>(compSpv),
    };
    VK_ASSERT(vkCreateShaderModule(device.device, &compInfo, nullptr, &comp));

    VkComputePipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = comp,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        .layout = layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VK_ASSERT(vkCreateComputePipelines(device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
    vkDestroyShaderModule(device.device, comp, nullptr);
}