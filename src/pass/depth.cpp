#define ARAWN_IMPLEMENTATION
#include "pass/depth.h"
#include "renderer.h"
#include "engine.h"
#include "settings.h"
#include "swapchain.h"
#include "vertex.h"
#include "model.h"
#include "camera.h"

DepthPass::DepthPass(Renderer& renderer) {
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

    { // create pipeline layout
        std::array<VkDescriptorSetLayout, 2> sets = { engine.camera_layout, engine.transform_layout };

        VkPipelineLayoutCreateInfo info{ 
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 
            sets.size(), sets.data(), 0, nullptr 
        };
        VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &layout));
    }


    VkSampleCountFlagBits sample_count = settings.sample_count();
    { // create renderpass
        VkAttachmentReference depth_ref = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
        VkAttachmentDescription depth_info = { 
            0, VK_FORMAT_D32_SFLOAT, sample_count, 
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
            VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        };
        VkSubpassDescription subpass{ 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 0, nullptr, nullptr, &depth_ref, 0, nullptr };
        VkRenderPassCreateInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 1, &depth_info, 1, &subpass, 0, nullptr };

        VK_ASSERT(vkCreateRenderPass(engine.device, &info, nullptr, &renderpass));
    }

    { // create pipeline
        VkShaderModule vert_module = engine.create_shader("res/import/shader/transform/basic.vert.spv");
        VkShaderModule frag_module = engine.create_shader("res/import/shader/empty.frag.spv");

        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
        shader_stages[0] = { 
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, 
            VK_SHADER_STAGE_VERTEX_BIT, vert_module, "main", nullptr 
        };

        shader_stages[1] = {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, 
            VK_SHADER_STAGE_FRAGMENT_BIT, frag_module, "main", nullptr
        };

        std::array<VkVertexInputBindingDescription, 1> vertex_binding;
        vertex_binding[0] = { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };

        std::array<VkVertexInputAttributeDescription, 1> vertex_attrib; // max vertex count
        vertex_attrib[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) };
        
        VkPipelineVertexInputStateCreateInfo vertex_state {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0, 
            vertex_binding.size(), vertex_binding.data(), vertex_attrib.size(), vertex_attrib.data()
        };

        VkPipelineInputAssemblyStateCreateInfo assembly_state {
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE
        };

        VkPipelineViewportStateCreateInfo viewport_state {
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0, 
            1, nullptr, 1, nullptr // viewport & scissor are dynamic
        };

        VkPipelineRasterizationStateCreateInfo rasterizer_state{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0,
            VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 
            VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f 
        };

        VkPipelineMultisampleStateCreateInfo multisampling_state{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0, 
            sample_count, VK_FALSE
        };

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0, 
            VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE
        };

        std::array<VkPipelineColorBlendAttachmentState, 0> attachment_blend;
        
        VkPipelineColorBlendStateCreateInfo colour_blending{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0, 
            VK_FALSE, VK_LOGIC_OP_COPY, attachment_blend.size(), attachment_blend.data(), { 0.0f, 0.0f, 0.0f, 0.0f }
        };

        std::array<VkDynamicState, 2> dynamics { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        
        VkPipelineDynamicStateCreateInfo dynamic_states{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, 
            dynamics.size(), dynamics.data()
        };

        VkGraphicsPipelineCreateInfo info{
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0, 
            shader_stages.size(), shader_stages.data(), &vertex_state, &assembly_state, nullptr, 
            &viewport_state, &rasterizer_state, &multisampling_state, &depth_stencil_state, 
            &colour_blending, &dynamic_states, layout, renderpass, 0, nullptr, 0  
        };

        VK_ASSERT(vkCreateGraphicsPipelines(engine.device, nullptr, 1, &info, nullptr, &pipeline));

        vkDestroyShaderModule(engine.device, vert_module, nullptr);
        vkDestroyShaderModule(engine.device, frag_module, nullptr);
    }

    { // create framebuffers
        std::array<VkImageView, 1> attachments;

        VkFramebufferCreateInfo info{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, 
            renderpass, attachments.size(), attachments.data(), 
            swapchain.extent.x, swapchain.extent.y, 1
        };

        for (uint32_t i = 0; i < settings.frame_count; ++i) {
            attachments[0] = renderer.depth_attachment[i].view;
            VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &framebuffer[i]));
        }
    }
}

DepthPass::~DepthPass() {
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyRenderPass(engine.device, renderpass, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        
        uint32_t i;
        for (i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkDestroyFramebuffer(engine.device, framebuffer[i], nullptr);
        }
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, i, cmd_buffer.data());

        cmd_buffer[0] = nullptr;
    }
}

DepthPass::DepthPass(DepthPass&& other) {
    if (this == &other) return;

    cmd_buffer = other.cmd_buffer;
    
    pipeline = other.pipeline;
    renderpass = other.renderpass;
    layout = other.layout;

    framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;
}

DepthPass& DepthPass::operator=(DepthPass&& other) {
    if (this == &other) return *this;
    
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyRenderPass(engine.device, renderpass, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        
        uint32_t i;
        for (i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkDestroyFramebuffer(engine.device, framebuffer[i], nullptr);
        }
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, i, cmd_buffer.data());

    }

    layout = other.layout;
    renderpass = other.renderpass;
    pipeline = other.pipeline;
    cmd_buffer = other.cmd_buffer;
    framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;

    return *this;
}    

void DepthPass::record(uint32_t frame_index) {

    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }

    { // begin renderpass
        VkClearValue clear;
        clear.depthStencil = { 1.0f, 0 };
        
        VkRenderPassBeginInfo info{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, 
            renderpass, framebuffer[frame_index], 
            { { 0, 0, }, { swapchain.extent.x, swapchain.extent.y } }, 
            1, &clear
        };
        vkCmdBeginRenderPass(cmd_buffer[frame_index], &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    { // bind pipeline
        vkCmdBindPipeline(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapchain.extent.x;
        viewport.height = (float) swapchain.extent.y;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buffer[frame_index], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { swapchain.extent.x, swapchain.extent.y };
        vkCmdSetScissor(cmd_buffer[frame_index], 0, 1, &scissor);
    }

    { // draw scene 
        // bind camera
        vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &camera.uniform[frame_index].descriptor_set, 0, nullptr);

        for (Model& model : models) {
            // bind vbo
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd_buffer[frame_index], 0, 1, &model.vertex_buffer, offsets);
            
            // bind ibo
            vkCmdBindIndexBuffer(cmd_buffer[frame_index], model.index_buffer, 0, VK_INDEX_TYPE_UINT32);
            
            // bind transform
            vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &model.transform.uniform[frame_index].descriptor_set, 0, nullptr);

            vkCmdDrawIndexed(cmd_buffer[frame_index], model.vertex_count, 1, 0, 0, 0);
        }
    }
    
    vkCmdEndRenderPass(cmd_buffer[frame_index]);

    //{ // memory barrier
    //    std::array<VkImageMemoryBarrier, 1> image_barriers;
    //    image_barriers[0] = { 
    //        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, 
    //        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, 
    //        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 
    //        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, 
    //        renderer.depth_attachment[frame_index].image, 
    //        { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }
    //    };
    //
    //    vkCmdPipelineBarrier(cmd_buffer[frame_index], 
    //        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 
    //        0, nullptr, 0, nullptr, image_barriers.size(), image_barriers.data()
    //    );
    //}

    VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
}

