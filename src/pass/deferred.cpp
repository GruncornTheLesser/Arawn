#define ARAWN_IMPLEMENTATION
#include "pass/deferred.h"
#include "renderer.h"
#include "engine.h"
#include "settings.h"
#include "swapchain.h"
#include "vertex.h"
#include "model.h"
#include "camera.h"



DeferredPass::DeferredPass(Renderer& renderer) {
    VkSampleCountFlagBits sample_count = settings.sample_count();
    
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
        std::array<VkDescriptorSetLayout, 3> sets{ engine.camera_layout, engine.transform_layout, engine.material_layout };
        
        VkPipelineLayoutCreateInfo info{ 
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 
            sets.size(), sets.data(), 0, nullptr
        };
        VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &layout));
    }

    { // create renderpass
        std::array<VkAttachmentDescription, 6> attachment_info;
        VkAttachmentReference depth_ref;
        std::array<VkAttachmentReference, 3>  colour_ref;
        
        VkSubpassDescription subpass {
            0, VK_PIPELINE_BIND_POINT_GRAPHICS,
            0, nullptr, 0, colour_ref.data(), 
            nullptr, &depth_ref, 0, nullptr
        };

        VkRenderPassCreateInfo info{
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 
            0, attachment_info.data(), 1, &subpass, 0, nullptr
        };

        /*
        attachment 0 depth attachment
        attachment 1 albedo attachment
        attachment 2 normal attachment
        attachment 3 specular attachment
        */

        // bindings
        { // create depth attachment reference
            if (settings.depth_prepass_enabled()) {
                depth_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
                VkAttachmentDescription& depth_info = attachment_info[depth_ref.attachment];
                depth_info = {
                    0, VK_FORMAT_D32_SFLOAT, sample_count, 
                    VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, 
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                }; 
            } else {
                depth_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
                VkAttachmentDescription& depth_info = attachment_info[depth_ref.attachment];
                depth_info = { 
                    0, VK_FORMAT_D32_SFLOAT, sample_count, 
                    VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                };
            }
        }
        
        { // create colour attachments
            VkAttachmentReference& albedo_ref = colour_ref[subpass.colorAttachmentCount++];
            albedo_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

            VkAttachmentDescription& albedo_info = attachment_info[albedo_ref.attachment];
            albedo_info = {
                0, VK_FORMAT_R8G8B8A8_UNORM, sample_count, 
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkAttachmentReference& normal_ref = colour_ref[subpass.colorAttachmentCount++];
            normal_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

            VkAttachmentDescription& normal_info = attachment_info[normal_ref.attachment];
            normal_info = {
                0, VK_FORMAT_R32G32B32A32_SFLOAT, sample_count, 
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkAttachmentReference& position_ref = colour_ref[subpass.colorAttachmentCount++];
            position_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

            VkAttachmentDescription& position_info = attachment_info[position_ref.attachment];
            position_info = {
                0, VK_FORMAT_R32G32B32A32_SFLOAT, sample_count, 
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };       
        }
        
        VK_ASSERT(vkCreateRenderPass(engine.device, &info, nullptr, &renderpass));
    }

    { // create pipeline
        VkShaderModule vert_module = engine.create_shader("res/import/shader/transform/tbn.vert.spv");
        VkShaderModule frag_module = engine.create_shader("res/import/shader/deferred/geometry.frag.spv");
        
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
        std::array<VkVertexInputAttributeDescription, 5> vertex_attrib;
        VkPipelineVertexInputStateCreateInfo vertex_state;

        vertex_binding[0] = { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };

        vertex_attrib[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) };
        vertex_attrib[1] = { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texcoord) };
        vertex_attrib[2] = { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) };
        vertex_attrib[3] = { 3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent) };
        vertex_attrib[4] = { 4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, bi_tangent) };

        vertex_state = {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0, 
            vertex_binding.size(), vertex_binding.data(), vertex_attrib.size(), vertex_attrib.data()
        };

        VkPipelineInputAssemblyStateCreateInfo assembly_state {
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0, 
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE
        };

        VkPipelineViewportStateCreateInfo viewport_state{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0,
            1, nullptr, 1, nullptr
        };

        VkPipelineRasterizationStateCreateInfo rasterizer_state{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0,
            VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 
            (settings.depth_prepass_enabled() ? VK_TRUE : VK_FALSE) , -5.0f, 0.0f, -1.1f, 1.0f
        };

        VkPipelineMultisampleStateCreateInfo multisampling_state{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0,
            sample_count, VK_FALSE
        };

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0,
            VK_TRUE, settings.depth_prepass_enabled() ? VK_FALSE : VK_TRUE,
            VK_COMPARE_OP_LESS_OR_EQUAL, VK_FALSE, VK_FALSE
        };

        std::array<VkPipelineColorBlendAttachmentState, 3> attachment_blend;
        attachment_blend[0] = { VK_FALSE };
        attachment_blend[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        attachment_blend[1] = { VK_FALSE };
        attachment_blend[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        attachment_blend[2] = { VK_FALSE };
        attachment_blend[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        
        VkPipelineColorBlendStateCreateInfo blend_state{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0, 
            VK_FALSE, VK_LOGIC_OP_COPY, attachment_blend.size(), attachment_blend.data()
        };

        std::array<VkDynamicState, 2> dynamics { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        
        VkPipelineDynamicStateCreateInfo dynamic_state{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, 
            dynamics.size(), dynamics.data()
        };

        VkGraphicsPipelineCreateInfo info{
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, nullptr, 0,
            shader_stages.size(), shader_stages.data(), &vertex_state, &assembly_state, nullptr, &viewport_state, 
            &rasterizer_state, &multisampling_state, &depth_stencil_state, &blend_state, &dynamic_state, 
            layout, renderpass
        };
        VK_ASSERT(vkCreateGraphicsPipelines(engine.device, nullptr, 1, &info, nullptr, &pipeline));

        vkDestroyShaderModule(engine.device, vert_module, nullptr);
        vkDestroyShaderModule(engine.device, frag_module, nullptr);
    }

    { // create framebuffers
        std::array<VkImageView, 4> attachments;

        VkFramebufferCreateInfo info{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, 
            renderpass, attachments.size(), attachments.data(), 
            swapchain.extent.x, swapchain.extent.y, 1  
        };

        for (uint32_t i = 0; i < settings.frame_count; ++i) {
            attachments[0] = renderer.depth_attachment[i].view;
            attachments[1] = renderer.albedo_attachment[i].view;
            attachments[2] = renderer.normal_attachment[i].view;
            attachments[3] = renderer.position_attachment[i].view;
            
            VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &framebuffer[i]));
        }
    }
}

DeferredPass::~DeferredPass() {
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

DeferredPass::DeferredPass(DeferredPass&& other) {
    if (this == &other) return;

    cmd_buffer = other.cmd_buffer;
    
    pipeline = other.pipeline;
    renderpass = other.renderpass;
    layout = other.layout;

    framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;
}

DeferredPass& DeferredPass::operator=(DeferredPass&& other) {
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

void DeferredPass::record(uint32_t frame_index) {
    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }

    { // begin renderpass
        std::array<VkClearValue, 4> clear;
        clear[0].depthStencil = { 1.0f, 0 }; // depth
        clear[1].color = { 0.0, 0.0, 0.0 };  // alebdo
        clear[2].color = { 0.0, 0.0, 0.0 };  // normal
        clear[3].color = { 0.0, 0.0, 0.0 };  // position

        VkRenderPassBeginInfo info{
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, 
            renderpass, framebuffer[frame_index], { { 0, 0 }, 
            { swapchain.extent.x, swapchain.extent.y } }, 
            clear.size(), clear.data()
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

            uint32_t index_offset = 0;
            for (auto& mesh : model.meshes) {
                // bind material
                vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2, 1, &mesh.material.uniform.descriptor_set, 0, nullptr);

                // draw mesh
                vkCmdDrawIndexed(cmd_buffer[frame_index], mesh.vertex_count, 1, index_offset, 0, 0);
                
                index_offset += mesh.vertex_count;
            }
        }
    }

    vkCmdEndRenderPass(cmd_buffer[frame_index]);
    
    VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
}

