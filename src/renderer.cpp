#define ARAWN_IMPLEMENTATION
#include "renderer.h"
#include "engine.h"
#include "swapchain.h"
#include "model.h"
#include "camera.h"

Renderer::Renderer() {
    recreate();
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

    msaa_attachment = std::move(other.msaa_attachment);
    depth_attachment = std::move(other.depth_attachment);
    albedo_attachment = std::move(other.albedo_attachment);
    normal_attachment = std::move(other.normal_attachment);
    position_attachment = std::move(other.position_attachment);
    cluster_buffer = std::move(other.cluster_buffer);
    light_buffer = std::move(other.light_buffer);

    uint32_t frame_index = other.frame_index;
    uint32_t frame_count = other.frame_count;

    input_attachment_set = std::move(other.input_attachment_set);
    image_available = std::move(other.image_available);
    in_flight = std::move(other.in_flight);

    depth_pass = std::move(other.depth_pass);
    culling_pass = std::move(other.culling_pass);
    deferred_pass = std::move(other.deferred_pass);
    forward_pass = std::move(other.forward_pass);
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

    input_attachment_set = std::move(other.input_attachment_set);
    image_available = std::move(other.image_available);
    in_flight = std::move(other.in_flight);

    depth_pass = std::move(other.depth_pass);
    culling_pass = std::move(other.culling_pass);
    deferred_pass = std::move(other.deferred_pass);
    forward_pass = std::move(other.forward_pass);

    return *this;
}

void Renderer::recreate() {
    vkDeviceWaitIdle(engine.device);
    
    swapchain.recreate();
    
    { // recreate attachments
        if (settings.culling_pass_enabled) {
            depth_attachment = TextureAttachment(
                settings.frame_count,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_FORMAT_D32_SFLOAT, 
                VK_IMAGE_ASPECT_DEPTH_BIT, 
                settings.sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                std::array<uint32_t, 2>{} = { engine.compute.family, engine.graphics.family }
            );
        } else {
            depth_attachment = TextureAttachment(
                settings.frame_count,
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
                settings.frame_count,
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

        if (settings.culling_pass_enabled) {

        } else {

        }

        if (settings.deferred_pass_enabled) {
            albedo_attachment = TextureAttachment(
                settings.frame_count,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 
                VK_FORMAT_R8G8B8A8_UNORM, 
                VK_IMAGE_ASPECT_COLOR_BIT, 
                settings.sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );

            normal_attachment = TextureAttachment(
                settings.frame_count,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_ASPECT_COLOR_BIT, 
                settings.sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );

            position_attachment = TextureAttachment( // metallic, roughness, ... some other stuff
                settings.frame_count,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 
                VK_FORMAT_R32G32B32A32_SFLOAT, // TODO: downsize to 16bit & normalize to camera
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
                    nullptr, 0, 0, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 
                    &image_info[0]
                };
                write_info[1] = { 
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, 
                    nullptr, 1, 0, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 
                    &image_info[1]
                };
                write_info[2] = { 
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, 
                    nullptr, 2, 0, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 
                    &image_info[2]
                };
                
                for (uint32_t i = 0; i < settings.frame_count; ++i) {
                    VK_ASSERT(vkAllocateDescriptorSets(engine.device, &alloc_info, &input_attachment_set[i]));

                    image_info[0] = { engine.sampler, albedo_attachment.view[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                    image_info[1] = { engine.sampler, normal_attachment.view[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                    image_info[2] = { engine.sampler, position_attachment.view[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

                    write_info[0].dstSet = input_attachment_set[i];
                    write_info[1].dstSet = input_attachment_set[i];
                    write_info[2].dstSet = input_attachment_set[i];

                    vkUpdateDescriptorSets(engine.device, 3, write_info.data(), 0, nullptr);
                }
            }

        } else {
            if (frame_count != 0) {
                VK_ASSERT(vkFreeDescriptorSets(engine.device, engine.descriptor_pool, frame_count, input_attachment_set.data()));
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
        
        current_version = 1;
        for (uint32_t i = 0; i < settings.frame_count; ++i) {
            frame_version[i] = 0;
        }
    }

    { // update buffer attachments
        light_buffer = BufferAttachment(
            settings.frame_count, 
            nullptr, 
            sizeof(uint32_t) + sizeof(Light) * MAX_LIGHTS, 
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            std::array<uint32_t, 2>() = { engine.compute.family, engine.graphics.family }
        );

        if (settings.culling_mode == CullingMode::TILED) {
            cell_count = glm::uvec3(settings.resolution / settings.tile_size + glm::uvec2(1), 1);
            uint32_t tile_count = cell_count.x * cell_count.y;

            cluster_buffer = BufferAttachment(
                settings.frame_count, 
                nullptr, 
                sizeof(Frustrum) * tile_count, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                std::array<uint32_t, 2>() = { engine.compute.family, engine.graphics.family }
            );

            culled_buffer = BufferAttachment(
                settings.frame_count, 
                nullptr, 
                sizeof(LightArray<MAX_LIGHTS_PER_TILE>) * tile_count,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                std::array<uint32_t, 2>() = { engine.compute.family, engine.graphics.family }
            );


        } else if (settings.culling_mode == CullingMode::CLUSTERED) {
            cell_count = glm::uvec3(settings.resolution / settings.tile_size + glm::uvec2(1), 8);
            uint32_t cluster_count = cell_count.x * cell_count.y * cell_count.z;

            cluster_buffer = BufferAttachment(
                settings.frame_count, 
                nullptr, 
                sizeof(Cluster) * cluster_count, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                std::array<uint32_t, 2>() = { engine.compute.family, engine.graphics.family }
            );

            culled_buffer = BufferAttachment(
                settings.frame_count, nullptr, sizeof(LightArray<MAX_LIGHTS_PER_CLUSTER>) * cluster_count,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                std::array<uint32_t, 2>() = { engine.compute.family, engine.graphics.family }
            );
        } else {
            cluster_buffer = BufferAttachment();
            culled_buffer = BufferAttachment();
        }

        {
            VkDescriptorSetAllocateInfo info{ 
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, 
                engine.descriptor_pool, 1, &engine.light_layout
            };

            for (uint32_t i = 0; i < settings.frame_count; ++i) {
                VK_ASSERT(vkAllocateDescriptorSets(engine.device, &info, &light_attachment_set[i]));

                VkDescriptorBufferInfo light_buffer_info{ light_buffer.buffer[i], 0, VK_WHOLE_SIZE };
                VkDescriptorBufferInfo culled_buffer_info{ culled_buffer.buffer[i], 0, VK_WHOLE_SIZE };
                VkDescriptorBufferInfo cell_buffer_info{ cluster_buffer.buffer[i], 0, VK_WHOLE_SIZE };
                
                std::array<VkWriteDescriptorSet, 3> info;
                VkWriteDescriptorSet& light_info = info[0];
                light_info = { 
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, 
                    light_attachment_set[i], 0, 0, 1, 
                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
                    nullptr, &light_buffer_info, nullptr 
                };
                
                if (settings.culling_mode != CullingMode::NONE) {
                    VkWriteDescriptorSet& culled_info = info[1];
                    culled_info = { 
                        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, 
                        light_attachment_set[i], 1, 0, 1, 
                        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
                        nullptr, &culled_buffer_info, nullptr 
                    };

                    VkWriteDescriptorSet& cell_binding = info[2];
                    cell_binding = { 
                        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, 
                        light_attachment_set[i], 2, 0, 1, 
                        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
                        nullptr, &cell_buffer_info, nullptr 
                    };
                    
                    vkUpdateDescriptorSets(engine.device, 3, info.data(), 0, nullptr);
                } else {
                    vkUpdateDescriptorSets(engine.device, 1, info.data(), 0, nullptr);
                }

            }
        }
    }
    frame_count = settings.frame_count;

}

void Renderer::record(uint32_t frame_index) {
    if (depth_pass.enabled())    depth_pass.record(frame_index);
    if (deferred_pass.enabled()) deferred_pass.record(frame_index);
    if (culling_pass.enabled())  culling_pass.record(frame_index);
    if (forward_pass.enabled())  forward_pass.record(frame_index);

    if (lights.size() > 0) { // update light buffer
        void* mapped_data;
        uint32_t data_size = sizeof(LightArray<MAX_LIGHTS>);
        VK_ASSERT(vkMapMemory(engine.device, light_buffer.memory[frame_index], 0, data_size, 0, &mapped_data));
        LightArray<MAX_LIGHTS>& light_array = *reinterpret_cast<LightArray<MAX_LIGHTS>*>(mapped_data);
        light_array.count = lights.size();
        std::memcpy(light_array.data, lights.data(), data_size);
        vkUnmapMemory(engine.device, light_buffer.memory[frame_index]);
    }
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
    
    { // update
        if (frame_version[frame_index] != current_version) {
            frame_version[frame_index] = current_version;
            record(frame_index);
        }
        
        camera.update(frame_index);
        
        for (auto& model : models) {
            model.transform.update(frame_index);
        }
    }

    VK_ASSERT(vkResetFences(engine.device, 1, &in_flight[frame_index]));
    
    if (settings.z_prepass_enabled) {
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