#define ARAWN_IMPLEMENTATION
#include "pass/culling.h"
#include "renderer.h"
#include "settings.h"
#include "engine.h"

CullingPass::CullingPass(Renderer& renderer) {
    { // create cmd buffers
        VkCommandBufferAllocateInfo info{ 
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, 
            engine.graphics.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, settings.frame_count
        };
        VK_ASSERT(vkAllocateCommandBuffers(engine.device, &info, cmd_buffer.data()));

        if (settings.frame_count != MAX_FRAMES_IN_FLIGHT) {
            cmd_buffer[settings.frame_count] = nullptr;
        }
    }

    { // create semaphores
        VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
        for (uint32_t i = 0; i < settings.frame_count; ++i) {
            VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &finished[i]));
        }
    }

    { // create pipeline layout
        std::array<VkDescriptorSetLayout, 2> sets = { engine.camera_layout, engine.light_layout };

        VkPipelineLayoutCreateInfo info{ 
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 
            sets.size(), sets.data(), 0, nullptr 
        };
        VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &layout));
    }

    { // create pipeline
        VkShaderModule comp_module;
        if (true) {
            comp_module = engine.create_shader("res/import/shader/cluster_culling.vert.spv");
        } else {
            comp_module = engine.create_shader("res/import/shader/tile_culling.vert.spv");
        }

        std::array<VkPipelineShaderStageCreateInfo, 1> shader_stages;
        shader_stages[0] = { 
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, 
            VK_SHADER_STAGE_COMPUTE_BIT, comp_module, "main", nullptr 
        };

        VkGraphicsPipelineCreateInfo info{
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0, 
            shader_stages.size(), shader_stages.data(), nullptr, nullptr, nullptr, 
            nullptr, nullptr, nullptr, nullptr, 
            nullptr, nullptr, layout, nullptr, 0, nullptr, 0  
        };

        VK_ASSERT(vkCreateGraphicsPipelines(engine.device, nullptr, 1, &info, nullptr, &pipeline));

        vkDestroyShaderModule(engine.device, comp_module, nullptr);
    }
}

CullingPass::~CullingPass() {
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        uint32_t i;
        for (i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkDestroySemaphore(engine.device, finished[i], nullptr);
        }
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, i, cmd_buffer.data());

        cmd_buffer[0] = nullptr;
    }
}

CullingPass::CullingPass(CullingPass&& other) {
    if (this == &other) return;

    cmd_buffer = other.cmd_buffer;
    finished = other.finished;
    
    pipeline = other.pipeline;
    layout = other.layout;

    //framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;
}

CullingPass& CullingPass::operator=(CullingPass&& other) {
    if (this == &other) return *this;

    std::swap(cmd_buffer, other.cmd_buffer);
    std::swap(finished, other.finished);
    
    std::swap(pipeline, other.pipeline);
    std::swap(layout, other.layout);

    //framebuffer = other.framebuffer;

    return *this;
}    

void CullingPass::record(uint32_t frame_index) {
    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }

    VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
}