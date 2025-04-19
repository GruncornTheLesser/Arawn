#define ARAWN_IMPLEMENTATION
#include "renderer.h"
#include "engine.h"
#include "swapchain.h"
#include "model.h"
#include "camera.h"

struct LightHeader { glm::uvec3 cluster_count; uint32_t light_count; };

Renderer::Renderer() {
    recreate();
}

Renderer::~Renderer() {
    if (!forward_pass.enabled()) return;
    
    vkWaitForFences(engine.device, frame_count, in_flight.data(), VK_TRUE, UINT64_MAX);

    for (uint32_t i = 0; i < frame_count; ++i) {  // destroy sync objects   
        vkDestroyFence(engine.device, in_flight[i], nullptr);
        vkDestroySemaphore(engine.device, image_ready[i], nullptr);
        vkDestroySemaphore(engine.device, frame_ready[i], nullptr);
        if (config.culling_enabled()) {
            vkDestroySemaphore(engine.device, light_ready[i], nullptr);
        }
        if (config.deferred_pass_enabled()) {
            vkDestroySemaphore(engine.device, defer_ready[i], nullptr);
        }
        if (config.depth_prepass_enabled()) {
            vkDestroySemaphore(engine.device, depth_ready[i * 2], nullptr);   
        }
        if (config.culling_mode() == CullingMode::TILED && config.deferred_pass_enabled() && config.depth_prepass_enabled()) {
            vkDestroySemaphore(engine.device, depth_ready[i * 2 + 1], nullptr);
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
    frustum_buffer = std::move(other.frustum_buffer);
    light_buffer = std::move(other.light_buffer);

    uint32_t frame_index = other.frame_index;
    uint32_t frame_count = other.frame_count;

    input_attachment_set = std::move(other.input_attachment_set);
    light_attachment_set = std::move(other.light_attachment_set);

    in_flight = std::move(other.in_flight);
    image_ready = std::move(other.image_ready);
    depth_ready = std::move(other.depth_ready);
    light_ready = std::move(other.light_ready);
    defer_ready = std::move(other.defer_ready);
    frame_ready = std::move(other.frame_ready);

    depth_pass = std::move(other.depth_pass);
    culling_pass = std::move(other.culling_pass);
    deferred_pass = std::move(other.deferred_pass);
    forward_pass = std::move(other.forward_pass);
}

Renderer& Renderer::operator=(Renderer&& other) {
    if (this == &other) return *this;
    
    if (forward_pass.enabled()) {
        vkWaitForFences(engine.device, frame_count, in_flight.data(), VK_TRUE, UINT64_MAX);

        for (uint32_t i = 0; i < frame_count; ++i) {  // destroy sync objects
            
            vkDestroyFence(engine.device, in_flight[i], nullptr);
            vkDestroySemaphore(engine.device, image_ready[i], nullptr);
            vkDestroySemaphore(engine.device, frame_ready[i], nullptr);
            if (config.culling_enabled()) {
                vkDestroySemaphore(engine.device, light_ready[i], nullptr);
            }
            if (config.deferred_pass_enabled()) {
                vkDestroySemaphore(engine.device, defer_ready[i], nullptr);
            }
            if (config.depth_prepass_enabled()) {
                vkDestroySemaphore(engine.device, depth_ready[i * 2], nullptr);   
            }
            if (config.culling_mode() == CullingMode::TILED && config.deferred_pass_enabled() && config.depth_prepass_enabled()) {
                vkDestroySemaphore(engine.device, depth_ready[i * 2 + 1], nullptr);
            }
            
        }
    }

    msaa_attachment = std::move(other.msaa_attachment);
    depth_attachment = std::move(other.depth_attachment);
    albedo_attachment = std::move(other.albedo_attachment);
    normal_attachment = std::move(other.normal_attachment);
    position_attachment = std::move(other.position_attachment);
    frustum_buffer = std::move(other.frustum_buffer);
    light_buffer = std::move(other.light_buffer);

    uint32_t frame_index = other.frame_index;
    uint32_t frame_count = other.frame_count;

    input_attachment_set = std::move(other.input_attachment_set);
    light_attachment_set = std::move(other.light_attachment_set);

    in_flight = std::move(other.in_flight);
    image_ready = std::move(other.image_ready);
    frame_ready = std::move(other.frame_ready);
    depth_ready = std::move(other.depth_ready);
    defer_ready = std::move(other.defer_ready);
    light_ready = std::move(other.light_ready);

    depth_pass = std::move(other.depth_pass);
    culling_pass = std::move(other.culling_pass);
    deferred_pass = std::move(other.deferred_pass);
    forward_pass = std::move(other.forward_pass);

    return *this;
}

void Renderer::recreate() {
    vkDeviceWaitIdle(engine.device);
    
    swapchain.recreate();

    VkSampleCountFlagBits sample_count = settings.sample_count();

    switch (settings.culling_mode()) {
        case CullingMode::CLUSTERED: {
            cluster_count = glm::uvec3((swapchain.extent - glm::uvec2(1)) / glm::uvec2(32, 32) + glm::uvec2(1), 12);
            break; 
        }
        case CullingMode::TILED: {
            cluster_count = glm::uvec3((swapchain.extent - glm::uvec2(1)) / glm::uvec2(16) + glm::uvec2(1), 1);
            break; 
        }
        case CullingMode::NONE: {
            cluster_count = glm::uvec3(1, 1, 1);
            break;
        }
    }

    for (uint32_t i = 0; i < settings.frame_count; ++i) { // recreate texture attachments
        // recreate depth attachment
        if (settings.culling_mode() == CullingMode::TILED) {
            depth_attachment[i] = Texture(
                nullptr, swapchain.extent.x, swapchain.extent.y, 1, VK_FORMAT_D32_SFLOAT, 
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_IMAGE_ASPECT_DEPTH_BIT, sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                std::array<uint32_t, 2>{} = { engine.compute.family, engine.graphics.family }
            );
        } else {
            depth_attachment[i] = Texture(
                nullptr, swapchain.extent.x, swapchain.extent.y, 1, VK_FORMAT_D32_SFLOAT, 
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_IMAGE_ASPECT_DEPTH_BIT, sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );
        }

        // recreate msaa staging attachment
        if (settings.msaa_enabled()) {
            msaa_attachment[i] = Texture(
                nullptr, swapchain.extent.x, swapchain.extent.y, 1, swapchain.format, 
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, 
                VK_IMAGE_ASPECT_COLOR_BIT, sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );
        } else {
            msaa_attachment[i] = Texture();
        }

        // recreate deferred texture attachments & descriptor set
        if (settings.render_mode() == RenderMode::DEFERRED) {
            albedo_attachment[i] = Texture(
                nullptr, swapchain.extent.x, swapchain.extent.y, 1, VK_FORMAT_R8G8B8A8_UNORM, 
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 
                VK_IMAGE_ASPECT_COLOR_BIT, sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );

            normal_attachment[i] = Texture(
                nullptr, swapchain.extent.x, swapchain.extent.y, 1, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 
                VK_IMAGE_ASPECT_COLOR_BIT, sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );

            // TODO: downsize to 16bit & normalize position to camera
            position_attachment[i] = Texture(
                nullptr, swapchain.extent.x, swapchain.extent.y, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 
                VK_IMAGE_ASPECT_COLOR_BIT, sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );

            input_attachment_set[i] = UniformSet(engine.attachment_layout, std::array<Uniform, 3>() = { 
                InputAttachment{ &albedo_attachment[i] }, 
                InputAttachment{ &normal_attachment[i] }, 
                InputAttachment{ &position_attachment[i] }
            });

        } else {
            albedo_attachment[i] = Texture();
            normal_attachment[i] = Texture();
            position_attachment[i] = Texture();
            input_attachment_set[i] = UniformSet();
        }
    }
    for (uint32_t i = settings.frame_count; i < frame_count; ++i) { // destroy additional attachments
        depth_attachment[i] = Texture();
        msaa_attachment[i] = Texture();
        light_buffer[i] = Buffer();
        cluster_buffer[i] = Buffer();
        albedo_attachment[i] = Texture();
        normal_attachment[i] = Texture();
        position_attachment[i] = Texture();

        light_attachment_set[i] = UniformSet();
        input_attachment_set[i] = UniformSet();
    }

    { // recreate light buffer attachments
        // recreate light buffer
        
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            light_buffer[i] = Buffer( 
                nullptr, sizeof(LightHeader) + sizeof(Light) * MAX_LIGHTS, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                std::array<uint32_t, 2>() = { engine.compute.family, engine.graphics.family }
            );
        }

        // recreate frustum and culled buffer
        if (settings.culling_enabled()) {
            uint32_t cell_count, cell_size;
            if (settings.culling_mode() == CullingMode::TILED) {
                cell_count = (swapchain.extent.x / TILE_SIZE + 1) * (swapchain.extent.y / TILE_SIZE + 1);
                cell_size = 1 + MAX_LIGHTS_PER_TILE;
            } else {
                cell_count = cluster_count.x * cluster_count.y * cluster_count.z;
                cell_size = 1 + MAX_LIGHTS_PER_CLUSTER;
            }
            
            frustum_buffer = Buffer( 
                nullptr, sizeof(Frustum) * cell_count, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                std::array<uint32_t, 2>() = { engine.compute.family, engine.graphics.family }
            );

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                cluster_buffer[i] = Buffer( 
                    nullptr, sizeof(uint32_t) * cell_size * cell_count,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                    std::array<uint32_t, 2>() = { engine.compute.family, engine.graphics.family }
                );
            }
        } else {
            frustum_buffer = Buffer();
            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                cluster_buffer[i] = Buffer();
            }
        }

        // recreate light descriptor set
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            switch (settings.culling_mode()) {
                case CullingMode::NONE: {
                    light_attachment_set[i] = UniformSet(engine.light_layout, std::array<Uniform, 1>() = { 
                            StorageBuffer{ &light_buffer[i] }
                    });
                    break;
                }
                case CullingMode::CLUSTERED: {
                    light_attachment_set[i] = UniformSet(engine.light_layout, std::array<Uniform, 3>() = { 
                        StorageBuffer{ &light_buffer[i] }, 
                        StorageBuffer{ &frustum_buffer }, 
                        StorageBuffer{ &cluster_buffer[i] }
                    });
                    break;
                }
                case CullingMode::TILED: {
                    light_attachment_set[i] = UniformSet(engine.light_layout, std::array<Uniform, 4>() = { 
                        StorageBuffer{ &light_buffer[i] }, 
                        StorageBuffer{ &frustum_buffer }, 
                        StorageBuffer{ &cluster_buffer[i] }, 
                        DepthAttachment{ &depth_attachment[i] }
                    });
                    break;
                }
            }
        }
        
    }

    { // recreate sync objects
        { // in flight fence
            VkFenceCreateInfo info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
            for (uint32_t i = frame_count; i < settings.frame_count; ++i) {
                VK_ASSERT(vkCreateFence(engine.device, &info, nullptr, &in_flight[i]));
            }
            for (uint32_t i = settings.frame_count; i < frame_count; ++i) {
                vkDestroyFence(engine.device, in_flight[i], nullptr);
            }
        }
        
        { // image ready & frame ready semaphore 
            VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
            for (uint32_t i = frame_count; i < settings.frame_count; ++i) {
                VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &image_ready[i]));
                VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &frame_ready[i]));
            }
            for (uint32_t i = settings.frame_count; i < frame_count; ++i) {
                vkDestroySemaphore(engine.device, image_ready[i], nullptr);
                vkDestroySemaphore(engine.device, frame_ready[i], nullptr);
            }
        }

        // depth ready semaphore
        if (settings.depth_mode() == DepthMode::ENABLED) {
            VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
            if (config.depth_prepass_enabled()) {
                for (uint32_t i = frame_count; i < settings.frame_count; ++i) { // create more
                    VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &depth_ready[i * 2]));
                }
                for (uint32_t i = settings.frame_count; i < frame_count; ++i) { // desrtoy extra
                    vkDestroySemaphore(engine.device, depth_ready[i * 2], nullptr);
                }
            } else {
                for (uint32_t i = 0; i < settings.frame_count; ++i) { // from old size to new, create new
                    VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &depth_ready[i * 2]));
                }
            }
        } else if (config.depth_prepass_enabled()) {
            for (uint32_t i = 0; i < frame_count; ++i) {
                vkDestroySemaphore(engine.device, depth_ready[i * 2], nullptr);
            }
        }

        // depth ready 2 semaphore
        if (settings.depth_mode() == DepthMode::ENABLED && settings.culling_mode() == CullingMode::TILED && settings.render_mode() == RenderMode::DEFERRED) {
            VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
            if (config.depth_prepass_enabled() && config.culling_mode() == CullingMode::TILED && config.deferred_pass_enabled()) {
                for (uint32_t i = frame_count; i < settings.frame_count; ++i) {
                    VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &depth_ready[i * 2 + 1]));
                }
                for (uint32_t i = settings.frame_count; i < frame_count; ++i) {
                    vkDestroySemaphore(engine.device, depth_ready[i * 2 + 1], nullptr);
                }
            } else {
                for (uint32_t i = 0; i < settings.frame_count; ++i) {
                    VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &depth_ready[i * 2 + 1]));
                }
            }
        } else if (config.depth_prepass_enabled() && config.culling_mode() == CullingMode::TILED && config.deferred_pass_enabled()) {
            for (uint32_t i = 0; i < frame_count; ++i) {
                vkDestroySemaphore(engine.device, depth_ready[i * 2 + 1], nullptr);
            }
        }

        // defer ready semaphore
        if (settings.render_mode() == RenderMode::DEFERRED) {
            VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
            if (config.deferred_pass_enabled()) {
                for (uint32_t i = frame_count; i < settings.frame_count; ++i) {
                    VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &defer_ready[i]));
                }
                for (uint32_t i = settings.frame_count; i < frame_count; ++i) {
                    vkDestroySemaphore(engine.device, defer_ready[i], nullptr);
                }
            } else {
                for (uint32_t i = 0; i < settings.frame_count; ++i) {
                    VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &defer_ready[i]));
                }
            }
        } else if (config.deferred_pass_enabled()) {
            for (uint32_t i = 0; i < frame_count; ++i) {
                vkDestroySemaphore(engine.device, defer_ready[i], nullptr);
            }
        }

        // light ready semaphore
        if (settings.culling_enabled()) {
            VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
            if (config.culling_enabled()) {
                for (uint32_t i = frame_count; i < settings.frame_count; ++i) {
                    VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &light_ready[i]));
                }
                for (uint32_t i = settings.frame_count; i < frame_count; ++i) {
                    vkDestroySemaphore(engine.device, light_ready[i], nullptr);
                }
            } else {
                for (uint32_t i = 0; i < settings.frame_count; ++i) {
                    VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &light_ready[i]));
                }
            
            }
        } else if (config.culling_enabled()) {
            for (uint32_t i = 0; i < frame_count; ++i) {
                vkDestroySemaphore(engine.device, light_ready[i], nullptr);
            }
        }

        current_version = 1;
        for (uint32_t i = 0; i < settings.frame_count; ++i) {
            frame_version[i] = 0;
        }
    }

    { // recreate passes
        if (settings.depth_prepass_enabled()) {
            depth_pass = DepthPass(*this);
        } else {
            depth_pass = DepthPass();
        }

        if (settings.render_mode() == RenderMode::DEFERRED) {
            deferred_pass = DeferredPass(*this);
        } else {
            deferred_pass = DeferredPass();
        }

        if (settings.culling_enabled()) {
            culling_pass = CullingPass(*this);
        } else {
            culling_pass = CullingPass();
        }

        forward_pass = ForwardPass(*this);
    }

    config = settings;
    frame_count = settings.frame_count;
}

void Renderer::record(uint32_t frame_index) {
    if (config.depth_prepass_enabled()) depth_pass.record(frame_index);
    if (config.deferred_pass_enabled()) deferred_pass.record(frame_index);
    if (config.culling_enabled())       culling_pass.record(frame_index);
    forward_pass.record(frame_index);

    if (lights.size() > 0) { // update light buffer
        LightHeader light_header { cluster_count, static_cast<uint32_t>(lights.size()) };
        light_buffer[frame_index].set_value(&light_header, sizeof(LightHeader));
        light_buffer[frame_index].set_value(lights.data(), sizeof(Light) * lights.size(), sizeof(LightHeader));
    }
}

void Renderer::draw() {
    const uint64_t timeout = 1000000000;
    
    VK_ASSERT(vkWaitForFences(engine.device, 1, &in_flight[frame_index], true, timeout));
    
    uint32_t image_index;
    VkResult res = vkAcquireNextImageKHR(engine.device, swapchain.swapchain, timeout, image_ready[frame_index], nullptr, &image_index);

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
    
    if (config.depth_prepass_enabled()) {
        /*
        waits on nothing, signals depth_ready, signals depth_ready_2 if enabled
        depth_ready_2 is only enabled when render_mode==deferred, culling_mode==tile and depth_prepass == true
        */
        VkSubmitInfo info{ 
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
            0, nullptr, nullptr, 
            1, &depth_pass.cmd_buffer[frame_index], 
            (config.deferred_pass_enabled() && config.culling_mode() == CullingMode::TILED) ? 2u : 1u, &depth_ready[frame_index * 2]
        };
        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, nullptr));
    }
    
    if (config.deferred_pass_enabled()) {
        VkPipelineStageFlags stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        VkSubmitInfo info{ 
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
            config.depth_prepass_enabled() ? 1u : 0u, &depth_ready[frame_index * 2], &stage, 
            1, &deferred_pass.cmd_buffer[frame_index], 
            1, &defer_ready[frame_index]
        };
        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, nullptr));
    }

    if (config.culling_enabled()) {
        /*
        wait on depth_ready_1 if culling_mode == tiled, render_mode == forward,  depth_prepass == true
        wait on depth_ready_2 if culling_mode == tiled, render_mode == deferred, depth_prepass == true
        wait on defer_ready   if culling_mode == tiled, render_mode == deferred, depth_prepass == false
        */

        VkPipelineStageFlags stage;
        VkSemaphore wait;
        VkSubmitInfo info{ 
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
            0, &wait, &stage, 
            1, &culling_pass.cmd_buffer[frame_index], 
            1, &light_ready[frame_index]
        };
        if (config.culling_mode() == CullingMode::TILED) {
            info.waitSemaphoreCount = 1;
            stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            if (config.render_mode() == RenderMode::DEFERRED) {
                if (config.depth_mode() == DepthMode::ENABLED) {
                    wait = depth_ready[frame_index * 2 + 1];
                } else {
                    wait = defer_ready[frame_index];
                }
            } else if (config.depth_mode() == DepthMode::ENABLED) {
                wait = depth_ready[frame_index * 2];
            }
        }
        
        VK_ASSERT(vkQueueSubmit(engine.compute.queue, 1, &info, nullptr));
    }

    { // forward pass
        /*
        wait on light if culling pass enabled
        wait_on defer if deferred pass enabled && !(culling_mode == tiled && !depth_enabled)
        wait on depth if depth enabled && culling pass != tiled
        */
        uint32_t wait_count = 0;
        std::array<VkSemaphore, 3> waits;
        std::array<VkPipelineStageFlags, 3> stages;

        if (config.culling_enabled()) {
            waits[wait_count] = light_ready[frame_index];
            stages[wait_count] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            ++wait_count;
        }
        
        if (config.deferred_pass_enabled()) {
            if (config.culling_mode() != CullingMode::TILED || config.depth_prepass_enabled()) {
                waits[wait_count] = defer_ready[frame_index];
                stages[wait_count] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                ++wait_count;
            }
        } else {
            if (config.depth_prepass_enabled() && config.culling_mode() != CullingMode::TILED) {
                waits[wait_count] = depth_ready[frame_index * 2];
                stages[wait_count] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                ++wait_count;
            }
        }
        waits[wait_count] = image_ready[frame_index];
        stages[wait_count] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        ++wait_count;

        VkSubmitInfo info{ 
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
            wait_count, waits.data(), stages.data(), 
            1, &forward_pass.cmd_buffer[frame_index], 
            1, &frame_ready[frame_index]
        };
        vkQueueSubmit(engine.graphics.queue, 1, &info, in_flight[frame_index]);
    }

    { // present to screen
        VkPresentInfoKHR info{ 
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr, 
            1, &frame_ready[frame_index], 
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