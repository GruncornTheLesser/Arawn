#define ARAWN_IMPLEMENTATION
#include "renderer.h"
#include "engine.h"
#include "swapchain.h"
#include "model.h"
#include "vertex.h"
#include "camera.h"
#include <fstream>
#include <numeric>

VkShaderModule create_shader_module(std::filesystem::path fp) {
    std::ifstream file(fp, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("could not open file: " + fp.string());

    std::vector<char> buffer(static_cast<std::size_t>(file.tellg()));
    file.seekg(0);
    file.read(buffer.data(), buffer.size());
    file.close();

    VkShaderModuleCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.pCode = reinterpret_cast<uint32_t*>(buffer.data());
    info.codeSize = buffer.size();

    VkShaderModule shader;
    VK_ASSERT(vkCreateShaderModule(engine.device, &info, NULL, &shader));
    return shader;
}

Renderer::Renderer() {
    recreate();
}

void Renderer::recreate() {
    vkDeviceWaitIdle(engine.device);
    
    swapchain.recreate();
    
    { // recreate attachments
        if (settings.culling_pass_enabled) {
            depth_attachment = TextureAttachment(
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_FORMAT_D32_SFLOAT, 
                VK_IMAGE_ASPECT_DEPTH_BIT, 
                settings.sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                std::array<uint32_t, 2>{} = { engine.compute.family, engine.graphics.family }
            );
        } else {
            depth_attachment = TextureAttachment(
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_FORMAT_D32_SFLOAT, 
                VK_IMAGE_ASPECT_DEPTH_BIT, 
                settings.sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );
        }

        if (settings.msaa_enabled) {
            msaa_attachment = TextureAttachment(
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
                swapchain.format, 
                VK_IMAGE_ASPECT_COLOR_BIT, 
                settings.sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );
        } else {
            msaa_attachment = TextureAttachment();
        }

        if (settings.deferred_pass_enabled) {
            albedo_attachment = TextureAttachment(
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 
                VK_FORMAT_R8G8B8A8_UNORM, 
                VK_IMAGE_ASPECT_COLOR_BIT, 
                settings.sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );

            normal_attachment = TextureAttachment(
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_ASPECT_COLOR_BIT, 
                settings.sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );

            position_attachment = TextureAttachment( // metallic, roughness, ... some other stuff
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 
                VK_FORMAT_R16G16B16A16_SFLOAT, 
                VK_IMAGE_ASPECT_COLOR_BIT, 
                settings.sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );

            { // create attachment descriptor set
                VkDescriptorSetAllocateInfo alloc_info{
                    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, 
                    engine.descriptor_pool, 1, &engine.attachment_layout
                };
                
                std::array<VkDescriptorImageInfo, 3> image_info;
                std::array<VkWriteDescriptorSet, 3> write_info;
                write_info[0] = { 
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, 
                    nullptr, 0, 0, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &image_info[0]
                };
                write_info[1] = { 
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, 
                    nullptr, 1, 0, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &image_info[1]
                };
                write_info[2] = { 
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, 
                    nullptr, 2, 0, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &image_info[2]
                };
                
                for (uint32_t i = 0; i < settings.frame_count; ++i) {
                    VK_ASSERT(vkAllocateDescriptorSets(engine.device, &alloc_info, &attachment_set[i]));

                    image_info[0] = { engine.sampler, albedo_attachment.view[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                    image_info[1] = { engine.sampler, normal_attachment.view[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                    image_info[2] = { engine.sampler, position_attachment.view[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

                    write_info[0].dstSet = attachment_set[i];
                    write_info[1].dstSet = attachment_set[i];
                    write_info[2].dstSet = attachment_set[i];

                    vkUpdateDescriptorSets(engine.device, 3, write_info.data(), 0, nullptr);
                }
            }

        } else {
            if (frame_count != 0) {
                VK_ASSERT(vkFreeDescriptorSets(engine.device, engine.descriptor_pool, frame_count, attachment_set.data()));
            }

            albedo_attachment = TextureAttachment();
            normal_attachment = TextureAttachment();
            position_attachment = TextureAttachment();
        }
    }

    { // recreate passes
        if (settings.z_prepass_enabled) {
            depth_pass = DepthPass(*this);
        } else {
            depth_pass = DepthPass();
        }

        if (settings.deferred_pass_enabled) {
            deferred_pass = DeferredPass(*this);
        } else {
            deferred_pass = DeferredPass();
        }

        if (settings.culling_pass_enabled && false) {
            culling_pass = CullingPass(*this);
        } else {
            culling_pass = CullingPass();
        }

        forward_pass = ForwardPass(*this);
    }

    { // recreate sync objects pass
        if (frame_count < settings.frame_count) {
            VkSemaphoreCreateInfo sempaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
            VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
            
            for (uint32_t i = frame_count; i < settings.frame_count; ++i) {
                VK_ASSERT(vkCreateSemaphore(engine.device, &sempaphore_info, nullptr, &image_available[i]));
                VK_ASSERT(vkCreateFence(engine.device, &fence_info, nullptr, &in_flight[i]));
            }
        }
        else if (settings.frame_count < frame_count) {
            for (uint32_t i = settings.frame_count; i < settings.frame_count; ++i) {
                vkDestroySemaphore(engine.device, image_available[i], nullptr);
                vkDestroyFence(engine.device, in_flight[i], nullptr);
            }
        }
    }

    frame_count = settings.frame_count;
}

Renderer::~Renderer() {
    if (!forward_pass.enabled()) return;
    
    vkWaitForFences(engine.device, frame_count, in_flight.data(), VK_TRUE, UINT64_MAX);

    { // destroy sync objects pass
        for (uint32_t i = 0; i < frame_count; ++i) {
            vkDestroySemaphore(engine.device, image_available[i], nullptr);
            vkDestroyFence(engine.device, in_flight[i], nullptr);
        }
    }
}

Renderer::Renderer(Renderer&& other) {
    if (this == &other) return;


}
Renderer& Renderer::operator=(Renderer&& other) {
    if (this == &other) return *this;
    
    if (forward_pass.enabled()) {
        vkWaitForFences(engine.device, frame_count, in_flight.data(), VK_TRUE, UINT64_MAX);

        { // destroy sync objects pass
            for (uint32_t i = 0; i < frame_count; ++i) {
                vkDestroySemaphore(engine.device, image_available[i], nullptr);
                vkDestroyFence(engine.device, in_flight[i], nullptr);
            }
        }
    }

    msaa_attachment = std::move(other.msaa_attachment);
    depth_attachment = std::move(other.depth_attachment);
    albedo_attachment = std::move(other.albedo_attachment);
    normal_attachment = std::move(other.normal_attachment);
    position_attachment = std::move(other.position_attachment);
    cluster_buffer = std::move(other.cluster_buffer);
    light_buffer = std::move(other.light_buffer);

    uint32_t frame_index = other.frame_index;
    uint32_t frame_count = other.frame_count;

    attachment_set = std::move(other.attachment_set);
    image_available = std::move(other.image_available);
    in_flight = std::move(other.in_flight);

    depth_pass = std::move(other.depth_pass);
    culling_pass = std::move(other.culling_pass);
    deferred_pass = std::move(other.deferred_pass);
    forward_pass = std::move(other.forward_pass);

    return *this;
}

void Renderer::draw() {
    const uint64_t timeout = 1000000000;
    
    VK_ASSERT(vkWaitForFences(engine.device, 1, &in_flight[frame_index], true, timeout));
    
    uint32_t image_index;
    VkResult res = vkAcquireNextImageKHR(engine.device, swapchain.swapchain, timeout, image_available[frame_index], nullptr, &image_index);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate();
        return;
    } else if (res != VK_SUBOPTIMAL_KHR) {
        VK_ASSERT(res);
    }
    
    // update ubos
    {
        camera.update(frame_index);
        
        for (auto& model : models) {
            model.transform.update(frame_index);
        }
    }

    VK_ASSERT(vkResetFences(engine.device, 1, &in_flight[frame_index]));
    
    if (settings.z_prepass_enabled) {
        depth_pass.record(frame_index);
        
        uint32_t wait_count = 0;
        std::array<VkSemaphore, 1> wait_semaphores;
        std::array<VkPipelineStageFlags, 1> wait_stages;
        
        { // wait on image available
            wait_semaphores[wait_count] = image_available[frame_index];
            wait_stages[wait_count] =  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; 
            ++wait_count;
        }
        
        VkSubmitInfo info{
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
            wait_count, wait_semaphores.data(), wait_stages.data(),
            1, &depth_pass.cmd_buffer[frame_index], 
            1, &depth_pass.finished[frame_index]
        };
        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, nullptr));
    }
    
    if (settings.deferred_pass_enabled) { // deferred pass 
        deferred_pass.record(frame_index);

        uint32_t wait_count = 0;
        std::array<VkSemaphore, 1> wait_semaphores;
        std::array<VkPipelineStageFlags, 1> wait_stages;

        if (settings.z_prepass_enabled) { // wait on depth pass
            wait_semaphores[wait_count] = depth_pass.finished[frame_index];
            wait_stages[wait_count] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            ++wait_count;
        } else { // first command
            wait_semaphores[wait_count] = image_available[frame_index];
            wait_stages[wait_count] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; 
            ++wait_count;
        }
               
        VkSubmitInfo info{
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
            1, wait_semaphores.data(), wait_stages.data(),
            1, &deferred_pass.cmd_buffer[frame_index], 
            1, &deferred_pass.finished[frame_index]
        };

        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, nullptr));
    }

    if (settings.culling_pass_enabled) { // cluster pass
        culling_pass.record(frame_index);

        uint32_t wait_count = 0;
        std::array<VkSemaphore, 1> wait_semaphores;
        std::array<VkPipelineStageFlags, 1> wait_stages;
        
        // wait for depth attachment
        if (settings.z_prepass_enabled) { 
            wait_semaphores[wait_count] = depth_pass.finished[frame_index];
            wait_stages[wait_count] =  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; 
            ++wait_count;
        } else { // if (settings.deferred_pass_enabled)  
            wait_semaphores[wait_count] = deferred_pass.finished[frame_index];
            wait_stages[wait_count] =  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; 
            ++wait_count;
        }

        VkSubmitInfo info{
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
            1, wait_semaphores.data(), wait_stages.data(),
            1, &culling_pass.cmd_buffer[frame_index],
            1, &culling_pass.finished[frame_index]
        };

        VK_ASSERT(vkQueueSubmit(engine.compute.queue, 1, &info, nullptr));
    }
    
    { // forward pass
        forward_pass.record(frame_index);

        uint32_t wait_count = 0;
        std::array<VkSemaphore, 2> wait_semaphores;
        std::array<VkPipelineStageFlags, 2> wait_stages;

        // note: deferred pass either waits on z_prepass or populates the depth_pass itself
        if (settings.deferred_pass_enabled) { // wait on deferred pass
            wait_semaphores[wait_count] = deferred_pass.finished[frame_index];
            wait_stages[wait_count] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            ++wait_count;
        } else if (settings.z_prepass_enabled) { // wait on depth pass
            wait_semaphores[wait_count] = depth_pass.finished[frame_index];
            wait_stages[wait_count] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            ++wait_count;
        } else { // first command
            wait_semaphores[wait_count] = image_available[frame_index];
            wait_stages[wait_count] =  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; 
            ++wait_count;
        }
        
        if (settings.culling_pass_enabled) { // wait on culling pass
            wait_stages[wait_count] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            wait_semaphores[wait_count] = culling_pass.finished[frame_index];
            ++wait_count;
        }

        VkSubmitInfo info{
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
            wait_count, wait_semaphores.data(), wait_stages.data(), 
            1, &forward_pass.cmd_buffer[frame_index], 
            1, &forward_pass.finished[frame_index]
        };
        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, in_flight[frame_index]));
    }
    
    { // present to screen
        VkPresentInfoKHR info{ 
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr, 
            1, &forward_pass.finished[frame_index], 
            1, &swapchain.swapchain, &image_index, 
            nullptr
        };
        VkResult res = vkQueuePresentKHR(engine.present.queue, &info);

        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            recreate();
            frame_index = (frame_index + 1) % settings.frame_count; // get next frame data set
        } else {
            VK_ASSERT(res);
        }
    }
    
    frame_index = (frame_index + 1) % settings.frame_count; // get next frame data set
}

Renderer::DepthPass::DepthPass(Renderer& renderer) {
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
        std::array<VkDescriptorSetLayout, 2> sets = { engine.camera_layout, engine.transform_layout };

        VkPipelineLayoutCreateInfo info{ 
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 
            sets.size(), sets.data(), 0, nullptr 
        };
        VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &layout));
    }

    { // create renderpass
        VkAttachmentReference depth_ref = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
        VkAttachmentDescription depth_info = { 
            0, VK_FORMAT_D32_SFLOAT, settings.sample_count, 
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
            VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };
        VkSubpassDescription subpass{ 0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, nullptr, 0, nullptr, nullptr, &depth_ref, 0, nullptr };
        VkRenderPassCreateInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 1, &depth_info, 1, &subpass, 0, nullptr };

        VK_ASSERT(vkCreateRenderPass(engine.device, &info, nullptr, &renderpass));
    }

    { // create pipeline
        VkShaderModule vert_module = create_shader_module("res/import/shader/transform_basic.vert.spv");
        VkShaderModule frag_module = create_shader_module("res/import/shader/depth.frag.spv");

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
            settings.sample_count, VK_FALSE
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
            attachments[0] = renderer.depth_attachment.view[i];
            VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &framebuffer[i]));
        }
    }
}

Renderer::DepthPass::~DepthPass() {
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyRenderPass(engine.device, renderpass, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        
        uint32_t i;
        for (i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkDestroySemaphore(engine.device, finished[i], nullptr);
            vkDestroyFramebuffer(engine.device, framebuffer[i], nullptr);
        }
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, i, cmd_buffer.data());

        cmd_buffer[0] = nullptr;
    }
}

Renderer::DepthPass::DepthPass(DepthPass&& other) {
    if (this == &other) return;

    cmd_buffer = other.cmd_buffer;
    finished = other.finished;
    
    pipeline = other.pipeline;
    renderpass = other.renderpass;
    layout = other.layout;

    framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;
}

Renderer::DepthPass& Renderer::DepthPass::operator=(DepthPass&& other) {
    if (this == &other) return *this;
    
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyRenderPass(engine.device, renderpass, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        
        uint32_t i;
        for (i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkDestroySemaphore(engine.device, finished[i], nullptr);
            vkDestroyFramebuffer(engine.device, framebuffer[i], nullptr);
        }
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, i, cmd_buffer.data());

    }

    layout = other.layout;
    renderpass = other.renderpass;
    pipeline = other.pipeline;
    cmd_buffer = other.cmd_buffer;
    finished = other.finished;
    framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;

    return *this;
}    

void Renderer::DepthPass::record(uint32_t frame_index) {

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
        vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &camera.uniform[frame_index].set.descriptor_set, 0, nullptr);

        for (Model& model : models) {
            // bind vbo
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd_buffer[frame_index], 0, 1, &model.vertex_buffer, offsets);
            
            // bind ibo
            vkCmdBindIndexBuffer(cmd_buffer[frame_index], model.index_buffer, 0, VK_INDEX_TYPE_UINT32);
            
            // bind transform
            vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &model.transform.uniform[frame_index].set.descriptor_set, 0, nullptr);

            vkCmdDrawIndexed(cmd_buffer[frame_index], model.vertex_count, 1, 0, 0, 0);
        }
    }
    
    { // end renderpass & cmd buffer
        vkCmdEndRenderPass(cmd_buffer[frame_index]);

        VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
    }
}



Renderer::CullingPass::~CullingPass() {
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyRenderPass(engine.device, renderpass, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        uint32_t i;
        for (i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkDestroySemaphore(engine.device, finished[i], nullptr);
        }
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, i, cmd_buffer.data());

        cmd_buffer[0] = nullptr;
    }
}

Renderer::CullingPass::CullingPass(CullingPass&& other) {
    if (this == &other) return;

    cmd_buffer = other.cmd_buffer;
    finished = other.finished;
    
    pipeline = other.pipeline;
    renderpass = other.renderpass;
    layout = other.layout;

    //framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;
}

Renderer::CullingPass& Renderer::CullingPass::operator=(CullingPass&& other) {
    if (this == &other) return *this;

    std::swap(cmd_buffer, other.cmd_buffer);
    std::swap(finished, other.finished);
    
    std::swap(pipeline, other.pipeline);
    std::swap(renderpass, other.renderpass);
    std::swap(layout, other.layout);

    //framebuffer = other.framebuffer;

    return *this;
}    

void Renderer::CullingPass::record(uint32_t frame_index) {
    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }

    VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
}



Renderer::DeferredPass::DeferredPass(Renderer& renderer) {
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
        std::array<VkDescriptorSetLayout, 3> sets{ engine.camera_layout, engine.transform_layout, engine.material_layout };
        std::array<VkPushConstantRange, 0> ranges;
        //ranges[0] = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants) };

        VkPipelineLayoutCreateInfo info{ 
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 
            sets.size(), sets.data(), ranges.size(), ranges.data()
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
            if (settings.z_prepass_enabled) {
                depth_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
                VkAttachmentDescription& depth_info = attachment_info[depth_ref.attachment];
                depth_info = {
                    0, VK_FORMAT_D32_SFLOAT, settings.sample_count, 
                    VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                }; 
            } else {
                depth_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
                VkAttachmentDescription& depth_info = attachment_info[depth_ref.attachment];
                depth_info = { 
                    0, VK_FORMAT_D32_SFLOAT, settings.sample_count, 
                    VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                };
            }
        }
        
        { // create colour attachments
            VkAttachmentReference& albedo_ref = colour_ref[subpass.colorAttachmentCount++];
            albedo_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

            VkAttachmentDescription& albedo_info = attachment_info[albedo_ref.attachment];
            albedo_info = {
                0, VK_FORMAT_R8G8B8A8_UNORM, settings.sample_count, 
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkAttachmentReference& normal_ref = colour_ref[subpass.colorAttachmentCount++];
            normal_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

            VkAttachmentDescription& normal_info = attachment_info[normal_ref.attachment];
            normal_info = {
                0, VK_FORMAT_R8G8B8A8_UNORM, settings.sample_count, 
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkAttachmentReference& position_ref = colour_ref[subpass.colorAttachmentCount++];
            position_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

            VkAttachmentDescription& position_info = attachment_info[position_ref.attachment];
            position_info = {
                0, VK_FORMAT_R16G16B16A16_SFLOAT, settings.sample_count, 
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };       
        }
        
        VK_ASSERT(vkCreateRenderPass(engine.device, &info, nullptr, &renderpass));
    }

    { // create pipeline
        VkShaderModule vert_module = create_shader_module("res/import/shader/transform.vert.spv");
        VkShaderModule frag_module = create_shader_module("res/import/shader/deferred.frag.spv");
        
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
            VK_TRUE, -5.0f, 0.0f, -1.1f, 1.0f
        };

        VkPipelineMultisampleStateCreateInfo multisampling_state{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0,
            settings.sample_count, VK_FALSE
        };

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0,
            VK_TRUE, settings.z_prepass_enabled ? VK_FALSE : VK_TRUE,
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
            attachments[0] = renderer.depth_attachment.view[i];
            attachments[1] = renderer.albedo_attachment.view[i];
            attachments[2] = renderer.normal_attachment.view[i];
            attachments[3] = renderer.position_attachment.view[i];
            
            VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &framebuffer[i]));
        }
    }
}

Renderer::DeferredPass::~DeferredPass() {
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyRenderPass(engine.device, renderpass, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        uint32_t i;
        for (i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkDestroySemaphore(engine.device, finished[i], nullptr);
            vkDestroyFramebuffer(engine.device, framebuffer[i], nullptr);
        }
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, i, cmd_buffer.data());

        cmd_buffer[0] = nullptr;
    }
}

Renderer::DeferredPass::DeferredPass(DeferredPass&& other) {
    if (this == &other) return;

    cmd_buffer = other.cmd_buffer;
    finished = other.finished;
    
    pipeline = other.pipeline;
    renderpass = other.renderpass;
    layout = other.layout;

    framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;
}

Renderer::DeferredPass& Renderer::DeferredPass::operator=(DeferredPass&& other) {
    if (this == &other) return *this;
    
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyRenderPass(engine.device, renderpass, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        
        uint32_t i;
        for (i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkDestroySemaphore(engine.device, finished[i], nullptr);
            vkDestroyFramebuffer(engine.device, framebuffer[i], nullptr);
        }
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, i, cmd_buffer.data());

    }

    layout = other.layout;
    renderpass = other.renderpass;
    pipeline = other.pipeline;
    cmd_buffer = other.cmd_buffer;
    finished = other.finished;
    framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;

    return *this;
}    

void Renderer::DeferredPass::record(uint32_t frame_index) {
    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }

    { // begin renderpass
        std::array<VkClearValue, 4> clear;
        clear[0].depthStencil = { 1.0f, 0 };
        clear[1].color = { 0.1, 0.0, 0.1, 0.0 };
        clear[2].color = { 0.0, 0.0, 0.0, 0.0 };
        clear[3].color = { 0.0, 0.0, 0.0, 0.0 };

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
        vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &camera.uniform[frame_index].set.descriptor_set, 0, nullptr);

        for (Model& model : models) {
            // bind vbo
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd_buffer[frame_index], 0, 1, &model.vertex_buffer, offsets);
            
            // bind ibo
            vkCmdBindIndexBuffer(cmd_buffer[frame_index], model.index_buffer, 0, VK_INDEX_TYPE_UINT32);
            
            // bind transform
            vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &model.transform.uniform[frame_index].set.descriptor_set, 0, nullptr);

            uint32_t index_offset = 0;
            for (auto& mesh : model.meshes) {
                // bind material
                vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2, 1, &mesh.material.set.descriptor_set, 0, nullptr);

                // draw mesh
                vkCmdDrawIndexed(cmd_buffer[frame_index], mesh.vertex_count, 1, index_offset, 0, 0);
                
                index_offset += mesh.vertex_count;
            }
        }
    }
    { // end renderpass & cmd_buffer
        vkCmdEndRenderPass(cmd_buffer[frame_index]);
        
        VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
    }

}



Renderer::ForwardPass::ForwardPass(Renderer& renderer) {
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
        if (settings.deferred_pass_enabled) {
            std::array<VkDescriptorSetLayout, 2> sets{ engine.camera_layout, engine.attachment_layout };
            std::array<VkPushConstantRange, 0> ranges;
            //ranges[0] = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants) };
        
            VkPipelineLayoutCreateInfo info{ 
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 
                sets.size(), sets.data(), ranges.size(), ranges.data()
            };
            
            VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &layout));
        } else {
            std::array<VkDescriptorSetLayout, 3> sets{ engine.camera_layout, engine.transform_layout, engine.material_layout };
            std::array<VkPushConstantRange, 0> ranges;
            //ranges[0] = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants) };
        
            VkPipelineLayoutCreateInfo info{ 
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 
                sets.size(), sets.data(), ranges.size(), ranges.data()
            };
            
            VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &layout));
        }

    }

    { // create renderpass
        std::array<VkAttachmentDescription, 6> attachment_info;
        VkAttachmentReference colour_ref, depth_ref, resolve_ref;
        std::array<VkAttachmentReference, 3>  input_ref;
        
        VkSubpassDescription subpass {
            0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 
            input_ref.data(), 1, &colour_ref, nullptr, &depth_ref, 0, nullptr
        };

        VkRenderPassCreateInfo info{
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 
            0, attachment_info.data(), 1, &subpass, 0, nullptr
        };

        /*
        attachment 0 always depth attachment,
        attachment 1 always colour attachment, -> can be msaa_staging_attachment or swapchain image depending if msaa enabled
        attachment 2 swapchain if msaa_enabled
        attachment 3+ input attachments 
        */

        // bindings
        { // create depth attachment reference
            depth_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
            
            VkAttachmentDescription& depth_info = attachment_info[depth_ref.attachment];
            if (settings.z_prepass_enabled || settings.deferred_pass_enabled) {
                depth_info = { 
                    0, VK_FORMAT_D32_SFLOAT, settings.sample_count, 
                    VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                }; 
            } else {
                depth_info = { 
                    0, VK_FORMAT_D32_SFLOAT, settings.sample_count, 
                    VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                };
            }
        }
        
        /*
        when msaa enabled, the renderpass writes first to the colour attachment and then resolves the 
        multi sampling into the resolve attachment. So, when msaa is enabled, renderpass maps the colour 
        to a msaa staging attachment and the resolve to the swapchain image
        */
        if (settings.msaa_enabled) {
            colour_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            
            VkAttachmentDescription& colour_info = attachment_info[colour_ref.attachment];
            colour_info = { 
                0, swapchain.format, settings.sample_count, 
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            resolve_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        
            VkAttachmentDescription& resolve_info = attachment_info[resolve_ref.attachment];
            resolve_info = { 
                0, swapchain.format, VK_SAMPLE_COUNT_1_BIT, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            };

            subpass.pResolveAttachments = &resolve_ref;
        } else {
            colour_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            
            VkAttachmentDescription& colour_info = attachment_info[colour_ref.attachment];
            colour_info = { 
                0, swapchain.format, settings.sample_count, 
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            };
        }


        if (settings.deferred_pass_enabled) { // create input attachments
            VkAttachmentReference& albedo_ref = input_ref[subpass.inputAttachmentCount++];
            albedo_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

            VkAttachmentDescription& albedo_info = attachment_info[albedo_ref.attachment];
            albedo_info = {
                0, VK_FORMAT_R8G8B8A8_UNORM, settings.sample_count, 
                VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkAttachmentReference& normal_ref = input_ref[subpass.inputAttachmentCount++];
            normal_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

            VkAttachmentDescription& normal_info = attachment_info[normal_ref.attachment];
            normal_info = {
                0, VK_FORMAT_R8G8B8A8_UNORM, settings.sample_count, 
                VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkAttachmentReference& position_ref = input_ref[subpass.inputAttachmentCount++];
            position_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

            VkAttachmentDescription& position_info = attachment_info[position_ref.attachment];
            position_info = {
                0, VK_FORMAT_R16G16B16A16_SFLOAT, settings.sample_count, 
                VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };       
        }
        
        VK_ASSERT(vkCreateRenderPass(engine.device, &info, nullptr, &renderpass));
    }

    { // create pipeline
        VkShaderModule vert_module;
        VkShaderModule frag_module;
        if (settings.deferred_pass_enabled) {
            vert_module = create_shader_module("res/import/shader/fullscreen.vert.spv");
            if (settings.msaa_enabled) {
                frag_module = create_shader_module("res/import/shader/present_msaa.frag.spv");
            } else {
                frag_module = create_shader_module("res/import/shader/present.frag.spv");
            }
        } else {
            vert_module = create_shader_module("res/import/shader/transform.vert.spv");
            frag_module = create_shader_module("res/import/shader/forward.frag.spv");
        }

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

        if (!settings.deferred_pass_enabled) {
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
        } else {
            vertex_state = {
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0, 
                0, nullptr, 0, nullptr
            };
        }

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
            VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f
        };

        VkPipelineMultisampleStateCreateInfo multisampling_state{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0,
            settings.sample_count, VK_FALSE
        };

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0,
            settings.z_prepass_enabled ? VK_FALSE : VK_TRUE, 
            (settings.z_prepass_enabled || settings.z_prepass_enabled) ? VK_FALSE : VK_TRUE, VK_COMPARE_OP_LESS, 
            VK_FALSE, VK_FALSE
        };

        std::array<VkPipelineColorBlendAttachmentState, 1> attachment_blend;
        attachment_blend[0] = { VK_FALSE };
        attachment_blend[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        
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
        std::array<VkImageView, 6> attachments;

        VkFramebufferCreateInfo info{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, 
            renderpass, 0, attachments.data(), 
            swapchain.extent.x, swapchain.extent.y, 1  
        };
        framebuffer.resize(std::lcm(settings.frame_count, swapchain.image_count));

        for (uint32_t i = 0; i < framebuffer.size(); ++i) {
            uint32_t frame_i = i % settings.frame_count;
            uint32_t image_i = i % swapchain.image_count;
            
            info.attachmentCount = 0;

            // depth attachment
            attachments[info.attachmentCount++] = renderer.depth_attachment.view[frame_i];
            
            if (settings.msaa_enabled) { // colour attachment
                attachments[info.attachmentCount++] = renderer.msaa_attachment.view[frame_i];
            }

            // colour/resolve attachment
            attachments[info.attachmentCount++] = swapchain.view[image_i];

            // input attachments
            if (settings.deferred_pass_enabled) {
                attachments[info.attachmentCount++] = renderer.albedo_attachment.view[frame_i];
                attachments[info.attachmentCount++] = renderer.normal_attachment.view[frame_i];
                attachments[info.attachmentCount++] = renderer.position_attachment.view[frame_i];
            }

            VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &framebuffer[i]));
        }
    }
}

Renderer::ForwardPass::~ForwardPass() {
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyRenderPass(engine.device, renderpass, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        uint32_t i;
        for (i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkDestroySemaphore(engine.device, finished[i], nullptr);
        }
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, i, cmd_buffer.data());
        
        for (i = 0; i < framebuffer.size(); ++i) {
            vkDestroyFramebuffer(engine.device, framebuffer[i], nullptr);
        }

        cmd_buffer[0] = nullptr;
    }
}

Renderer::ForwardPass::ForwardPass(ForwardPass&& other) {
    if (this == &other) return;

    cmd_buffer = other.cmd_buffer;
    finished = other.finished;
    
    pipeline = other.pipeline;
    renderpass = other.renderpass;
    layout = other.layout;

    framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;
}

Renderer::ForwardPass& Renderer::ForwardPass::operator=(ForwardPass&& other) {
    if (this == &other) return *this;
    
    if (cmd_buffer[0] != nullptr) {
        vkDestroyPipelineLayout(engine.device, layout, nullptr);
        vkDestroyRenderPass(engine.device, renderpass, nullptr);
        vkDestroyPipeline(engine.device, pipeline, nullptr);
        
        uint32_t i;
        for (i = 0 ; i < MAX_FRAMES_IN_FLIGHT && cmd_buffer[i] != nullptr; ++i) {
            vkDestroySemaphore(engine.device, finished[i], nullptr);
        }
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, i, cmd_buffer.data());
        
        for (i = 0; i < framebuffer.size(); ++i) {
            vkDestroyFramebuffer(engine.device, framebuffer[i], nullptr);
        }
    }

    layout = other.layout;
    renderpass = other.renderpass;
    pipeline = other.pipeline;
    cmd_buffer = other.cmd_buffer;
    finished = other.finished;
    framebuffer = other.framebuffer;

    other.cmd_buffer[0] = nullptr;

    return *this;
}      

void Renderer::ForwardPass::record(uint32_t frame_index) {
    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }

    { // begin renderpass
        std::array<VkClearValue, 2> clear;
        clear[0].depthStencil = { 1.0f, 0 };
        clear[1].color = { 0.1, 0.01, 0.1 };

        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = renderpass;
        info.renderArea.offset = { 0, 0 };
        info.renderArea.extent = { swapchain.extent.x, swapchain.extent.y };
        info.framebuffer = framebuffer[frame_index];
        info.pClearValues = clear.data();
        info.clearValueCount = clear.size();

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

    if (settings.deferred_pass_enabled) {
        // bind camera
        vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &camera.uniform[frame_index].set.descriptor_set, 0, nullptr);
        
        // bind input attachments
        vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &renderer.attachment_set[frame_index], 0, nullptr);

        // draw fullscreen quad(vertices embedded in fullscreen.vert)
        vkCmdDraw(cmd_buffer[frame_index], 6, 1, 0, 0);

    } else { // draw scene
        // bind camera
        vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &camera.uniform[frame_index].set.descriptor_set, 0, nullptr);

        for (Model& model : models) {
            // bind vbo
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd_buffer[frame_index], 0, 1, &model.vertex_buffer, offsets);
            
            // bind ibo
            vkCmdBindIndexBuffer(cmd_buffer[frame_index], model.index_buffer, 0, VK_INDEX_TYPE_UINT32);
            
            // bind transform
            vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &model.transform.uniform[frame_index].set.descriptor_set, 0, nullptr);

            uint32_t index_offset = 0;
            for (auto& mesh : model.meshes) {
                // bind material
                vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2, 1, &mesh.material.set.descriptor_set, 0, nullptr);

                // draw mesh
                vkCmdDrawIndexed(cmd_buffer[frame_index], mesh.vertex_count, 1, index_offset, 0, 0);
                
                index_offset += mesh.vertex_count;
            }
        }
    }
    
    { // end render pass & cmd buffer
        vkCmdEndRenderPass(cmd_buffer[frame_index]);

        VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
    }
}