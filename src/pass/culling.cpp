#define ARAWN_IMPLEMENTATION
#include "pass/culling.h"
#include "renderer.h"
#include "settings.h"
#include "engine.h"
#include "camera.h"

CullingPass::CullingPass(Renderer& renderer) {
    { // create cmd buffers
        VkCommandBufferAllocateInfo info{ 
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, 
            engine.compute.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, settings.frame_count
        };
        VK_ASSERT(vkAllocateCommandBuffers(engine.device, &info, cmd_buffer.data()));

        if (settings.frame_count != MAX_FRAMES_IN_FLIGHT) {
            cmd_buffer[settings.frame_count] = nullptr;
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
        if (settings.culling_mode() == CullingMode::TILED) {
            if (settings.msaa_enabled()) {
                comp_module = engine.create_shader("res/import/shader/cull/tiled_ms.comp.spv");
            } else {
                comp_module = engine.create_shader("res/import/shader/cull/tiled.comp.spv");
            }
        } else  { // clustered
            comp_module = engine.create_shader("res/import/shader/cull/clustered.comp.spv");
        }

        VkComputePipelineCreateInfo info{
            VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0, 
            { 
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, 
                VK_SHADER_STAGE_COMPUTE_BIT, comp_module, "main", nullptr 
            }, 
            layout, nullptr, 0
        };

        VK_ASSERT(vkCreateComputePipelines(engine.device, nullptr, 1, &info, nullptr, &pipeline));

        vkDestroyShaderModule(engine.device, comp_module, nullptr);
    }

    {
        VkFence cmd_finished;
        {
            VkFenceCreateInfo info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
            VK_ASSERT(vkCreateFence(engine.device, &info, nullptr, &cmd_finished));
        }

        VkPipeline tmp_pipeline;
        { // create pipeline
            VkShaderModule comp_module = engine.create_shader("res/import/shader/cull/frustum.comp.spv");

            VkComputePipelineCreateInfo info{
                VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr, 0, 
                { 
                    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, 
                    VK_SHADER_STAGE_COMPUTE_BIT, comp_module, "main", nullptr 
                }, 
                layout, nullptr, 0
            };

            VK_ASSERT(vkCreateComputePipelines(engine.device, nullptr, 1, &info, nullptr, &tmp_pipeline));

            vkDestroyShaderModule(engine.device, comp_module, nullptr);
        }
        
        { // record temporary command
            camera.update(0);

            VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
            VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[0], &info));

            vkCmdBindPipeline(cmd_buffer[0], VK_PIPELINE_BIND_POINT_COMPUTE, tmp_pipeline);

            std::array<VkDescriptorSet, 2> sets = { camera.uniform[0].descriptor_set, renderer.light_attachment_set[0].descriptor_set };
            vkCmdBindDescriptorSets(cmd_buffer[0], VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 2, sets.data(), 0, nullptr);
            
            vkCmdDispatch(cmd_buffer[0], renderer.cluster_count.x, renderer.cluster_count.y, renderer.cluster_count.z);

            VK_ASSERT(vkEndCommandBuffer(cmd_buffer[0]));
        }
        
        {
            VkSubmitInfo info{VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, 1, &cmd_buffer[0], 0, nullptr };
            VK_ASSERT(vkQueueSubmit(engine.compute.queue, 1, &info, cmd_finished));

            VK_ASSERT(vkWaitForFences(engine.device, 1, &cmd_finished, VK_TRUE, UINT64_MAX));
        }

        { // destroy temporary objects
            vkDestroyFence(engine.device, cmd_finished, nullptr);
            vkDestroyPipeline(engine.device, tmp_pipeline, nullptr);
        }
    }
}

CullingPass::~CullingPass() {
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkFreeCommandBuffers(engine.device, engine.compute.pool, 1, &cmd_buffer[i]);
        }
        cmd_buffer[0] = nullptr;
    }
}

CullingPass::CullingPass(CullingPass&& other) {
    if (this == &other) return;

    cmd_buffer = other.cmd_buffer;
    
    pipeline = other.pipeline;
    layout = other.layout;

    other.cmd_buffer[0] = nullptr;
}

CullingPass& CullingPass::operator=(CullingPass&& other) {
    if (this == &other) return *this;

    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        for (uint32_t i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkFreeCommandBuffers(engine.device, engine.compute.pool, 1, &cmd_buffer[i]);
        }
    }

    cmd_buffer = other.cmd_buffer;
    
    pipeline = other.pipeline;
    layout = other.layout;

    other.cmd_buffer[0] = nullptr;
    
    return *this;
}    

void CullingPass::record(uint32_t frame_index) {
    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }
    
    { // dispatch culling
        vkCmdBindPipeline(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        std::array<VkDescriptorSet, 2> sets = { camera.uniform[frame_index].descriptor_set, renderer.light_attachment_set[frame_index].descriptor_set };
        vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, sets.size(), sets.data(), 0, nullptr);

        vkCmdDispatch(cmd_buffer[frame_index], renderer.cluster_count.x, renderer.cluster_count.y, renderer.cluster_count.z);
    }

    { // release cluster buffer
        VkBufferMemoryBarrier barrier{
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr, 
            VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, 0,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_EXTERNAL,
            renderer.cluster_buffer[frame_index].buffer, 0, VK_WHOLE_SIZE 
        };

        vkCmdPipelineBarrier(cmd_buffer[frame_index], 
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  
            0, 0, nullptr, 1, &barrier, 0, nullptr
        );
    }

    VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
}