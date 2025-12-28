#pragma once
#include <graphics/engine.h>
#include <graphics/swapchain.h>
#include <memory_resource>

namespace Arawn::Render {
    
    struct Resource {
        enum Type { IMAGE, BUFFER, ATTACHMENT } type : 8;
        uint32_t levels : 8, x : 16, y : 16, z : 16;
        VK_ENUM(VkFormat) format;
        union {
            VK_TYPE(VkImage) image;
            VK_TYPE(VkBuffer) buffer;
        };
        VK_TYPE(VkDeviceMemory) memory;
    };
    
    class Task {
        /*
        VkSubmitInfo info {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .pNext = nullptr, 
            .waitSemaphoreCount = 0, .pWaitSemaphores = nullptr, .pWaitDstStageMask = nullptr, 
            .commandBufferCount = 0, .pCommandBuffers = nullptr, 
            .signalSemaphoreCount = 0, .pSignalSemaphores = nullptr
        };

        vkQueueSubmit(engine.graphics.queue, 1, &info, renderer.in_flight[frame_index]);
        */
    };

    struct Builder {
    private:
        std::pmr::monotonic_buffer_resource cache;
    public:
        std::pmr::vector<Resource> resources{ &cache };
        std::pmr::vector<Task>     tasks{ &cache };
    };

    class Graph {    
        struct Task {
            enum { PRESENT, GRAPHICS, COMPUTE, ASYNC, TRANSFER } queueType : 3;

        };
        
        struct Node {
            uint32_t waitCount, emitCount;
            VK_TYPE(VkSemaphore)* wait;
            VK_TYPE(VkSemaphore)* emit;
        };

        uint32_t frameCount, frameIndex, imageCount, imageIndex;
        VK_TYPE(VkSemaphore)* frameReady;
        VK_TYPE(VkFence)*     imageReady;

        std::pmr::monotonic_buffer_resource cache;
        std::pmr::vector<VK_TYPE(VkFence)> fences{ &cache };
        std::pmr::vector<VK_TYPE(VkSemaphore)> semaphores{ &cache };
        std::pmr::vector<VK_TYPE(VkPipeline)> pipelines{ &cache };
        std::pmr::vector<VK_TYPE(VkPipelineLayout)> layouts{ &cache };
        std::pmr::vector<VK_TYPE(VkImageView)> imageViews{ &cache };
        std::pmr::vector<VK_TYPE(VkRenderPass)> renderpasses{ &cache };
        std::pmr::vector<VK_TYPE(VkFramebuffer)> framebuffers{ &cache };
        std::pmr::vector<VK_TYPE(VkCommandBuffer)> commands{ &cache };
        std::pmr::vector<VK_TYPE(VkDescriptorSet)> descriptorSets{ &cache };

        Graph(Builder& builder, uint32_t frameCount, uint32_t imageCount)
         : frameCount(frameCount), frameIndex(0), imageCount(imageCount), imageIndex(0)
        {
            
        }

        void render() {
            
            { // acquire image 
                // const uint64_t timeout = 1000000000;
                // VK_ASSERT(vkWaitForFences(engine.device, 1, &frameReady[frame.index], true, timeout));
                
                // acquire next swapchain image
                // VkResult res = vkAcquireNextImageKHR(engine.device, swapchain.swapchain, timeout, imageReady[frame.index], nullptr, &image.index);
            
                // { // recreate if out of date
                //     if (res == VK_ERROR_OUT_OF_DATE_KHR)
                //     {
                //         recreate();
                //         return;
                //     }
                //     else if (res != VK_SUBOPTIMAL_KHR)
                //     {
                //         VK_ASSERT(res);
                //     }
                // }
            }

            { // record command buffers
                // multithread here, each component can record to a seperate worker
            }

            { // submit command buffers
                // ...
            }

            { // present to screen
                // VkPresentInfoKHR info{ 
                //     VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr, 
                //     1, &frame.ready[frame.index], // wait on frame ready
                //     1, &swapchain.swapchain, &image.index, 
                //     nullptr
                // };
                // VkResult res = vkQueuePresentKHR(engine.present.queue, &info);
        
                // if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
                //     return;
                // } else {
                //     VK_ASSERT(res);
                // }
            }
        }
    };

    class Forward {
    public:
        Forward(Swapchain& swapchain);
        ~Forward();
        Forward(Forward&&);
        Forward& operator=(Forward&&);

        Forward(const Forward&) = delete;
        Forward& operator=(const Forward&) = delete;
        
        void record();
        void submit();

    private:
        // pipeline
        struct {
            VK_TYPE(VkPipeline) pipeline;
            VK_TYPE(VkPipelineLayout) layout;
        } pipeline;

        // sync primitives
        struct {
            VK_TYPE(VkSemaphore) semaphore[MAX_FRAMES_IN_FLIGHT];
            VK_TYPE(VkFence) fence[MAX_FRAMES_IN_FLIGHT];
        } sync;
        
        // pass information
        struct {
            VK_TYPE(VkRenderpass) renderpass;
            VK_TYPE(VkFramebuffer) framebuffer;
            VK_TYPE(VkCommandBuffer) cmd[MAX_FRAMES_IN_FLIGHT];
        } pass;
    };
}




struct Texture;
struct Buffer;

class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(Renderer&&);
    Renderer& operator=(Renderer&&);
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void draw();

    void recreate();

private:
    struct { uint32_t x, y, z; } cluster_count;
    uint32_t frame_index = 0;
    uint32_t frame_count = 0;

    // Texture msaa_attachment[MAX_FRAMES_IN_FLIGHT]; 
    // Texture depth_attachment[MAX_FRAMES_IN_FLIGHT]; 
    // Texture albedo_attachment[MAX_FRAMES_IN_FLIGHT]; 
    // Texture normal_attachment[MAX_FRAMES_IN_FLIGHT]; 
    // Texture position_attachment[MAX_FRAMES_IN_FLIGHT];
    // Buffer light_attachment[MAX_FRAMES_IN_FLIGHT];
    // Buffer cluster_attachment[MAX_FRAMES_IN_FLIGHT];
    // Buffer frustum_buffer;
    
    VK_TYPE(VkDescriptorSet) light_set[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkDescriptorSet) defer_set[MAX_FRAMES_IN_FLIGHT];

    VK_TYPE(VkFence)         in_flight[MAX_FRAMES_IN_FLIGHT];

    VK_TYPE(VkSemaphore) image_ready[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore) frame_ready[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore) depth_ready_graphic[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore) depth_ready_compute[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore) light_ready[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore) defer_ready[MAX_FRAMES_IN_FLIGHT];
};

/* headers
class CullingPass {
public:
    CullingPass() { cmd_buffer[0] = nullptr; }
    CullingPass(Renderer& renderer);
    ~CullingPass();
    CullingPass(CullingPass&& other);
    CullingPass& operator=(CullingPass&& other);
    CullingPass(const CullingPass& other) = delete;
    CullingPass& operator=(const CullingPass& other) = delete;

    void submit(uint32_t frame_index);
    void record(uint32_t frame_index, uint32_t current_version);
    bool enabled() { return cmd_buffer[0] != nullptr; }
    
    std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
    VK_TYPE(VkPipeline) pipeline;
    VK_TYPE(VkPipelineLayout) layout;
    std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> version;
};

class DeferredPass { 
public:
    DeferredPass() { cmd_buffer[0] = nullptr; }
    DeferredPass(Renderer& renderer);
    ~DeferredPass();
    DeferredPass(DeferredPass&& other);
    DeferredPass& operator=(DeferredPass&& other);
    DeferredPass(const DeferredPass& other) = delete;
    DeferredPass& operator=(const DeferredPass& other) = delete;
    
    void submit(uint32_t frame_index);
    void record(uint32_t frame_index, uint32_t current_version);
    bool enabled() { return cmd_buffer[0] != nullptr; }

    std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
    VK_TYPE(VkPipeline) pipeline;
    VK_TYPE(VkRenderPass) renderpass;
    VK_TYPE(VkPipelineLayout) layout;
    std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;
    std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> version;
};

class DepthPass {
public:
    DepthPass() { cmd_buffer[0] = nullptr; }
    DepthPass(Renderer& renderer);
    ~DepthPass();
    DepthPass(DepthPass&& other);
    DepthPass& operator=(DepthPass&& other);
    DepthPass(const DepthPass& other) = delete;
    DepthPass& operator=(const DepthPass& other) = delete;

    void submit(uint32_t frame_index);
    void record(uint32_t frame_index, uint32_t current_version);
    bool enabled() { return cmd_buffer[0] != nullptr; }

    std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
    VK_TYPE(VkPipeline) pipeline;
    VK_TYPE(VkRenderPass) renderpass;
    VK_TYPE(VkPipelineLayout) layout;

    std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;
    std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> version;
};

class ForwardPass {
public:
    ForwardPass() { }
    ForwardPass(Renderer& renderer);
    ~ForwardPass();
    ForwardPass(ForwardPass&& other);
    ForwardPass& operator=(ForwardPass&& other);
    ForwardPass(const ForwardPass& other) = delete;
    ForwardPass& operator=(const ForwardPass& other) = delete;

    void submit(uint32_t image_index, uint32_t frame_index);
    void record(uint32_t image_index, uint32_t frame_index, uint32_t current_version);
    bool enabled() { return !data.empty(); }

    VK_TYPE(VkPipeline) pipeline;
    VK_TYPE(VkRenderPass) renderpass;
    VK_TYPE(VkPipelineLayout) layout;

    // unlike the culling/depth/deferred pass forward pass must create a unique image view per swapchain image
    // swapchain image is not guaranteed to match the frame count.
    struct PresentData {
        VK_TYPE(VkImageView) view;
        std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
        std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;
        std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> version;
    };

    std::vector<PresentData> data;
};
*/

/*
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
        { // create fence
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
            
            vkCmdDispatch(cmd_buffer[0], renderer.cluster_count.x, renderer.cluster_count.y, 1);

            VK_ASSERT(vkEndCommandBuffer(cmd_buffer[0]));
        }
        
        { // submit initial command
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

void CullingPass::submit(uint32_t frame_index) {
    if (!enabled()) return; 

    std::array<VkSemaphore, 1> waits;
    std::array<VkPipelineStageFlags, 1> stages;
    VkSubmitInfo info{ 
        VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
        0, waits.data(), stages.data(), 
        1, &cmd_buffer[frame_index], 
        1, &renderer.light_ready[frame_index]
    };
    
    // tiled mode requires the depth attachment to be ready
    if (renderer.config.culling_mode() == CullingMode::TILED) {
        // if depth mode is enabled the culling pass waits on that 
        if (renderer.config.depth_mode() == DepthMode::ENABLED) {
            // if render mode is deferred wait on 2nd depth ready
            // else first
            if (renderer.config.render_mode() == RenderMode::DEFERRED) {
                waits[info.waitSemaphoreCount] = renderer.depth_ready[frame_index * 2 + 1];
            } else {
                waits[info.waitSemaphoreCount] = renderer.depth_ready[frame_index * 2];
            }
        } else {
            waits[info.waitSemaphoreCount] = renderer.defer_ready[frame_index];
        }
        stages[info.waitSemaphoreCount] = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        ++info.waitSemaphoreCount;
    }
    
    VK_ASSERT(vkQueueSubmit(engine.compute.queue, 1, &info, nullptr));
}

void CullingPass::record(uint32_t frame_index, uint32_t current_version) {
    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }

    { // dispatch culling
        vkCmdBindPipeline(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        std::array<VkDescriptorSet, 2> sets = { camera.uniform[frame_index].descriptor_set, renderer.light_attachment_set[frame_index].descriptor_set };
        vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, sets.size(), sets.data(), 0, nullptr);

        vkCmdDispatch(cmd_buffer[frame_index], renderer.cluster_count.x, renderer.cluster_count.y, renderer.cluster_count.z);
    }
    
    VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
}



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
                0, VK_FORMAT_R16G16B16A16_SFLOAT, sample_count, 
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkAttachmentReference& position_ref = colour_ref[subpass.colorAttachmentCount++];
            position_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

            VkAttachmentDescription& position_info = attachment_info[position_ref.attachment];
            position_info = {
                0, VK_FORMAT_R16G16B16A16_SFLOAT, sample_count, 
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

void DeferredPass::submit(uint32_t frame_index) {
    if (!enabled()) return;
    
    // waits on depth pre pass if enabled, signals defer ready
    std::array<VkSemaphore, 1> waits;
    std::array<VkPipelineStageFlags, 1> stages;
    VkSubmitInfo info{ 
        VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
        0, waits.data(), stages.data(),
        1, &cmd_buffer[frame_index], 
        1, &renderer.defer_ready[frame_index]
    };

    if (renderer.config.depth_prepass_enabled()) {
        // wait on depth ready
        waits[info.waitSemaphoreCount] = renderer.depth_ready[frame_index * 2];
        stages[info.waitSemaphoreCount] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        ++info.waitSemaphoreCount;
    }

    VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, nullptr));
}

void DeferredPass::record(uint32_t frame_index, uint32_t current_version) {
    if (!enabled()) return;
    if (version[frame_index] == current_version) return;

    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
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

        VkViewport viewport{ 0.0f, 0.0f, (float) swapchain.extent.x, (float) swapchain.extent.y, 0.0f, 1.0f };
        vkCmdSetViewport(cmd_buffer[frame_index], 0, 1, &viewport);

        VkRect2D scissor{ { 0, 0 }, { swapchain.extent.x, swapchain.extent.y }};
        vkCmdSetScissor(cmd_buffer[frame_index], 0, 1, &scissor);
    }

    { // draw scene
        // bind camera
        vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &camera.uniform[frame_index].descriptor_set, 0, nullptr);

        for (Model& model : models) {
            // bind vbo
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd_buffer[frame_index], 0, 1, &model.vertex.buffer, offsets);
            
            // bind ibo
            vkCmdBindIndexBuffer(cmd_buffer[frame_index], model.index.buffer, 0, VK_INDEX_TYPE_UINT32);
            
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

void DepthPass::submit(uint32_t frame_index) {
    if (!enabled()) return;

    VkSubmitInfo info{ 
        VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
        0, nullptr, nullptr,
        1, &renderer.depth_pass.cmd_buffer[frame_index], 
        1, &renderer.depth_ready[frame_index * 2]
    };

    // both deferred and culling wait on depth_ready but on different queues, requires seperate semaphores
    if (renderer.config.deferred_pass_enabled() && renderer.config.culling_mode() == CullingMode::TILED) {
        info.signalSemaphoreCount++;
    }

    VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, nullptr));
}

void DepthPass::record(uint32_t frame_index, uint32_t current_version) {
    if (!enabled()) return;
    if (version[frame_index] == current_version) return;

    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
        
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

        VkViewport viewport{ 0.0f, 0.0f, (float) swapchain.extent.x, (float) swapchain.extent.y, 0.0f, 1.0f };
        vkCmdSetViewport(cmd_buffer[frame_index], 0, 1, &viewport);

        VkRect2D scissor{ { 0, 0 }, { swapchain.extent.x, swapchain.extent.y } };
        vkCmdSetScissor(cmd_buffer[frame_index], 0, 1, &scissor);
    }

    { // draw scene 
        // bind camera
        vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &camera.uniform[frame_index].descriptor_set, 0, nullptr);

        for (Model& model : models) {
            // bind vbo
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd_buffer[frame_index], 0, 1, &model.vertex.buffer, offsets);
            
            // bind ibo
            vkCmdBindIndexBuffer(cmd_buffer[frame_index], model.index.buffer, 0, VK_INDEX_TYPE_UINT32);
            
            // bind transform
            vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &model.transform.uniform[frame_index].descriptor_set, 0, nullptr);

            vkCmdDrawIndexed(cmd_buffer[frame_index], model.vertex_count, 1, 0, 0, 0);
        }
    }
    
    vkCmdEndRenderPass(cmd_buffer[frame_index]);

    VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
}



ForwardPass::ForwardPass(Renderer& renderer) {   
    { // create pipeline layout
        if (settings.deferred_pass_enabled()) {
            std::array<VkDescriptorSetLayout, 3> sets{ engine.camera_layout, engine.attachment_layout, engine.light_layout };
            
            VkPipelineLayoutCreateInfo info{ 
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 
                sets.size(), sets.data(), 0, nullptr
            };
            
            VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &layout));
        } else {
            std::array<VkDescriptorSetLayout, 4> sets{ engine.camera_layout, engine.transform_layout, engine.material_layout, engine.light_layout };
            
            VkPipelineLayoutCreateInfo info{ 
                VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr, 0, 
                sets.size(), sets.data(), 0, nullptr
            };
            
            VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &layout));
        }
    }

    VkSampleCountFlagBits sample_count = settings.sample_count();

    { // create renderpass
        std::array<VkAttachmentDescription, 6> attachment_info;
        VkAttachmentReference colour_ref, depth_ref, resolve_ref;
        std::array<VkAttachmentReference, 3>  input_ref;
        
        VkSubpassDescription subpass {
            0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, 
            input_ref.data(), 1, &colour_ref, nullptr, nullptr, 0, nullptr
        };

        VkRenderPassCreateInfo info{
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr, 0, 
            0, attachment_info.data(), 1, &subpass, 0, nullptr
        };

        attachment 0 always depth attachment,
        attachment 1 always colour attachment, -> can be msaa_staging_attachment or swapchain image depending if msaa enabled
        attachment 2 swapchain if msaa_enabled
        attachment 3+ input attachments 

        // bindings
        if (!settings.deferred_pass_enabled()) { // create depth attachment reference
            subpass.pDepthStencilAttachment = &depth_ref;
            depth_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
            
            VkAttachmentDescription& depth_info = attachment_info[depth_ref.attachment];
            if (settings.depth_prepass_enabled()) {
                depth_info = { 
                    0, VK_FORMAT_D32_SFLOAT, sample_count, 
                    VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                }; 
            } else {
                depth_info = { 
                    0, VK_FORMAT_D32_SFLOAT, sample_count, 
                    VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                };
            }
        }
        
        
        // when msaa enabled, the renderpass writes first to the colour attachment and then resolves the 
        // multi sampling into the resolve attachment. So, when msaa is enabled, renderpass maps the colour 
        // to a msaa staging attachment and the resolve to the swapchain image

        if (settings.msaa_enabled()) {
            colour_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
            
            VkAttachmentDescription& colour_info = attachment_info[colour_ref.attachment];
            colour_info = { 
                0, swapchain.format, sample_count, 
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
                0, swapchain.format, sample_count, 
                VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            };
        }


        if (settings.deferred_pass_enabled()) { // create input attachments
            VkAttachmentReference& albedo_ref = input_ref[subpass.inputAttachmentCount++];
            albedo_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

            VkAttachmentDescription& albedo_info = attachment_info[albedo_ref.attachment];
            albedo_info = {
                0, VK_FORMAT_R8G8B8A8_UNORM, sample_count, 
                VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkAttachmentReference& normal_ref = input_ref[subpass.inputAttachmentCount++];
            normal_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

            VkAttachmentDescription& normal_info = attachment_info[normal_ref.attachment];
            normal_info = {
                0, VK_FORMAT_R16G16B16A16_SFLOAT, sample_count, 
                VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE, 
                VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            };

            VkAttachmentReference& position_ref = input_ref[subpass.inputAttachmentCount++];
            position_ref = { info.attachmentCount++, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

            VkAttachmentDescription& position_info = attachment_info[position_ref.attachment];
            position_info = {
                0, VK_FORMAT_R16G16B16A16_SFLOAT, sample_count, 
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
        if (settings.deferred_pass_enabled()) {
            vert_module = engine.create_shader("res/import/shader/transform/fullscreen.vert.spv");

            switch (settings.culling_mode()) {
                case CullingMode::TILED: {
                    if (settings.msaa_enabled()) frag_module = engine.create_shader("res/import/shader/deferred/tiled_ms.frag.spv");
                    else                         frag_module = engine.create_shader("res/import/shader/deferred/tiled.frag.spv");
                    break;
                }
                case CullingMode::CLUSTERED: {
                    if (settings.msaa_enabled()) frag_module = engine.create_shader("res/import/shader/deferred/clustered_ms.frag.spv");
                    else                         frag_module = engine.create_shader("res/import/shader/deferred/clustered.frag.spv");
                    break;
                }
                default: { // case CullingMode::DISABLED
                    if (settings.msaa_enabled()) frag_module = engine.create_shader("res/import/shader/deferred/present_ms.frag.spv");
                    else                         frag_module = engine.create_shader("res/import/shader/deferred/present.frag.spv");
                    break;
                }
            }
        } else {
            vert_module = engine.create_shader("res/import/shader/transform/tbn.vert.spv");
            switch (settings.culling_mode()) {
                case CullingMode::TILED: {
                    frag_module = engine.create_shader("res/import/shader/forward/tiled.frag.spv");
                    break;
                }
                case CullingMode::CLUSTERED: {
                    frag_module = engine.create_shader("res/import/shader/forward/clustered.frag.spv");
                    break;
                }
                default: { // case CullingMode::DISABLED
                    frag_module = engine.create_shader("res/import/shader/forward/standard.frag.spv");
                    break;
                }
            }
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

        if (!settings.deferred_pass_enabled()) {
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
            (settings.depth_prepass_enabled() && !settings.deferred_pass_enabled()) ? VK_TRUE : VK_FALSE, -5.0f, 0.0f, -1.1f, 1.0f
        };

        VkPipelineMultisampleStateCreateInfo multisampling_state{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0,
            sample_count, VK_FALSE
        };

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0,
            settings.deferred_pass_enabled() ? VK_FALSE : VK_TRUE, 
            (settings.depth_prepass_enabled() || settings.deferred_pass_enabled()) ? VK_FALSE : VK_TRUE, VK_COMPARE_OP_LESS, 
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

    { // init swapchain view
        uint32_t image_count;
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain.swapchain, &image_count, nullptr));
        std::vector<VkImage> images(image_count);
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain.swapchain, &image_count, images.data()));
        
        data.resize(image_count);

        VkImageViewCreateInfo info{
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0, 
            nullptr, VK_IMAGE_VIEW_TYPE_2D, swapchain.format, { }, 
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };
        
        for (uint32_t i = 0; i < images.size(); ++i) {
            info.image = images[i];
            VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &data[i].view));
        }
    }

    { // create cmd buffers
        VkCommandBufferAllocateInfo info{ 
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, 
            engine.graphics.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, settings.frame_count
        };
        for (auto& image_set : data) {
            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &info, image_set.cmd_buffer.data()));
        }

        if (settings.frame_count != MAX_FRAMES_IN_FLIGHT) {
            for (auto& image_set : data) {
                image_set.cmd_buffer[settings.frame_count] = nullptr;
            }
        }
    }

    { // create framebuffers
        std::array<VkImageView, 6> attachments;

        VkFramebufferCreateInfo info{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, nullptr, 0, 
            renderpass, 0, attachments.data(), 
            swapchain.extent.x, swapchain.extent.y, 1  
        };

        for (uint32_t frame_i = 0; frame_i < settings.frame_count; ++frame_i) {
            for (uint32_t image_i = 0; image_i < data.size(); ++image_i) {           
                info.attachmentCount = 0;

                // depth attachment
                if (!settings.deferred_pass_enabled()) {
                    attachments[info.attachmentCount++] = renderer.depth_attachment[frame_i].view;
                }

                if (settings.msaa_enabled()) { // colour attachment
                    attachments[info.attachmentCount++] = renderer.msaa_attachment[frame_i].view;
                }

                // colour/resolve attachment
                attachments[info.attachmentCount++] = data[image_i].view;

                // input attachments
                if (settings.deferred_pass_enabled()) {
                    attachments[info.attachmentCount++] = renderer.albedo_attachment[frame_i].view;
                    attachments[info.attachmentCount++] = renderer.normal_attachment[frame_i].view;
                    attachments[info.attachmentCount++] = renderer.position_attachment[frame_i].view;
                }

                VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &data[image_i].framebuffer[frame_i]));
            }
        }
    }
}

void ForwardPass::submit(uint32_t image_index, uint32_t frame_index) {
    // waits on image ready, if culling enabled waits on light_ready, if defer enabled waits on deferred
    // waits on depth if enabled and defer not enabled, signals frame ready
    
    std::array<VkSemaphore, 3> waits;
    std::array<VkPipelineStageFlags, 3> stages;
    VkSubmitInfo info{ 
        VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
        0, waits.data(), stages.data(), 
        1, &data[image_index].cmd_buffer[frame_index], 
        1, &renderer.frame_ready[frame_index]
    };
    
    { // wait on swapchain image
        waits[info.waitSemaphoreCount] = renderer.image_ready[frame_index];
        stages[info.waitSemaphoreCount] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        ++info.waitSemaphoreCount;
    }

    if (renderer.config.culling_enabled()) { // wait for light buffer
        waits[info.waitSemaphoreCount] = renderer.light_ready[frame_index];
        stages[info.waitSemaphoreCount] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        ++info.waitSemaphoreCount;
    }
    
    if (renderer.config.deferred_pass_enabled()) { // wait for g buffer attachments
        if (renderer.config.depth_prepass_enabled() || renderer.config.culling_mode() != CullingMode::TILED) {
            waits[info.waitSemaphoreCount] = renderer.defer_ready[frame_index];
            stages[info.waitSemaphoreCount] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            ++info.waitSemaphoreCount;
        }
    }
    else
    { // wait depth attachment
        if (renderer.config.depth_prepass_enabled() && renderer.config.culling_mode() != CullingMode::TILED) {
            waits[info.waitSemaphoreCount] = renderer.depth_ready[frame_index * 2];
            stages[info.waitSemaphoreCount] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            ++info.waitSemaphoreCount;
        }
    }
    
    vkQueueSubmit(engine.graphics.queue, 1, &info, renderer.in_flight[frame_index]);
}

void ForwardPass::record(uint32_t image_index, uint32_t frame_index, uint32_t current_version) {
    if (!enabled()) return;
    if (data[image_index].version[frame_index] == current_version) return;
    
    VkCommandBuffer cmd_buffer = data[image_index].cmd_buffer[frame_index];
    VkFramebuffer framebuffer = data[image_index].framebuffer[frame_index];

    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer, 0));
        
        VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer, &info));
    }

    { // begin renderpass
        std::array<VkClearValue, 2> clear;
        clear[0].depthStencil = { 1.0f, 0 };
        clear[1].color = { 0.0, 0.0, 0.0 };

        VkRenderPassBeginInfo info{ 
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr, 
            renderpass, framebuffer, 
            { { 0, 0 }, { swapchain.extent.x, swapchain.extent.y } }, 
            clear.size(), clear.data()
        };

        vkCmdBeginRenderPass(cmd_buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    { // bind pipeline
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkViewport viewport{ 0.0f, 0.0f, (float)swapchain.extent.x, (float)swapchain.extent.y, 0.0f, 1.0f };
        vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

        VkRect2D scissor{ { 0, 0 }, { swapchain.extent.x, swapchain.extent.y } };
        vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
    }

    if (settings.deferred_pass_enabled()) {
        std::array<VkDescriptorSet, 3> sets = {
            camera.uniform[frame_index].descriptor_set, 
            renderer.input_attachment_set[frame_index].descriptor_set, 
            renderer.light_attachment_set[frame_index].descriptor_set 
        };
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, sets.size(), sets.data(), 0, nullptr);

        // draw fullscreen quad(vertices embedded in fullscreen.vert)
        vkCmdDraw(cmd_buffer, 6, 1, 0, 0);

    } else { // forward pass
        // bind camera
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &camera.uniform[frame_index].descriptor_set, 0, nullptr);

        // bind lights
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 3, 1, &renderer.light_attachment_set[frame_index].descriptor_set, 0, nullptr);

        for (Model& model : models) {
            // bind vbo
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &model.vertex.buffer, offsets);
            
            // bind ibo
            vkCmdBindIndexBuffer(cmd_buffer, model.index.buffer, 0, VK_INDEX_TYPE_UINT32);
            
            // bind transform
            vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &model.transform.uniform[frame_index].descriptor_set, 0, nullptr);

            uint32_t index_offset = 0;
            for (auto& mesh : model.meshes) {
                // bind material
                vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2, 1, &mesh.material.uniform.descriptor_set, 0, nullptr);

                // draw mesh
                vkCmdDrawIndexed(cmd_buffer, mesh.vertex_count, 1, index_offset, 0, 0);
                
                index_offset += mesh.vertex_count;
            }
        }
    }
    
    { // end render pass & cmd buffer
        vkCmdEndRenderPass(cmd_buffer);

        VK_ASSERT(vkEndCommandBuffer(cmd_buffer));
    }
}



void Renderer::recreate() {
    // if already initialized, wait for frames to finish rendering
    if (forward_pass.enabled()) {
        VK_ASSERT(vkWaitForFences(engine.device, frame_count, in_flight.data(), VK_TRUE, UINT64_MAX));
    }
    // recreate swapchain values
    swapchain.recreate();
    VkSampleCountFlagBits sample_count = settings.sample_count();

    switch (settings.culling_mode()) {
        case CullingMode::CLUSTERED: {
            cluster_count = glm::uvec3(glm::ceil(glm::vec2(swapchain.extent) / glm::vec2(32)), 12);
            break; 
        }
        case CullingMode::TILED: {
            cluster_count = glm::uvec3(glm::ceil(glm::vec2(swapchain.extent) / glm::vec2(16)), 1);
            break; 
        }
        default: { // CullingMode::DISABLED
            cluster_count = glm::uvec3(1, 1, 1);
            break;
        }
    }

    for (uint32_t i = 0; i < settings.frame_count; ++i) { // recreate texture attachments
        // recreate depth attachment
        if (settings.culling_enabled()) {
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
                nullptr, swapchain.extent.x, swapchain.extent.y, 1, VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, 
                VK_IMAGE_ASPECT_COLOR_BIT, sample_count,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                { }
            );

            position_attachment[i] = Texture(
                nullptr, swapchain.extent.x, swapchain.extent.y, 1, VK_FORMAT_R16G16B16A16_SFLOAT, 
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

        // recreate frustum and cluster buffer
        if (settings.culling_enabled()) {
            
            uint32_t cell_size;
            if (settings.culling_mode() == CullingMode::TILED) {
                cell_size = 1 + MAX_LIGHTS_PER_TILE;
            } else {
                cell_size = 1 + MAX_LIGHTS_PER_CLUSTER;
            }
            
            frustum_buffer = Buffer( 
                nullptr, sizeof(Frustum) * cluster_count.x * cluster_count.y, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                std::array<uint32_t, 2>() = { engine.compute.family, engine.graphics.family }
            );

            for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                cluster_buffer[i] = Buffer( 
                    nullptr, sizeof(uint32_t) * cell_size * cluster_count.x * cluster_count.y * cluster_count.z,
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
                case CullingMode::CLUSTERED: {
                    light_attachment_set[i] = UniformSet(engine.light_layout, std::array<Uniform, 4>() = { 
                        StorageBuffer{ &light_buffer[i] }, 
                        StorageBuffer{ &frustum_buffer }, 
                        StorageBuffer{ &cluster_buffer[i] }, 
                        DepthAttachment{ &depth_attachment[i] }
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
                default: { // case CullingMode::DISABLED
                    light_attachment_set[i] = UniformSet(engine.light_layout, std::array<Uniform, 1>() = { 
                        StorageBuffer{ &light_buffer[i] }
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
    }

    if (settings.culling_enabled()) { // write cluster count to light buffer for frustum creation
        LightHeader light_header { cluster_count, static_cast<uint32_t>(lights.size()) };
        light_buffer[0].set_value(&light_header, sizeof(LightHeader));
    }

    { // recreate passes
        if (settings.depth_prepass_enabled()) {
            depth_pass = DepthPass(*this);
        } else {
            depth_pass = DepthPass();
        }

        if (settings.deferred_pass_enabled()) {
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

void Renderer::draw() {
    const uint64_t timeout = 1000000000;
    VK_ASSERT(vkWaitForFences(engine.device, 1, &in_flight[frame_index], true, timeout));
    
    // acquire next swapchain image
    uint32_t image_index;
    VkResult res = vkAcquireNextImageKHR(engine.device, swapchain.swapchain, timeout, image_ready[frame_index], nullptr, &image_index);

    { // recreate if out of date
        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate();
            return;
        } else if (res != VK_SUBOPTIMAL_KHR) {
            VK_ASSERT(res);
        }
    }

    { // update scene
        camera.update(frame_index);
        
        for (auto& model : models) {
            model.transform.update(frame_index);
        }

        LightHeader light_header { cluster_count, static_cast<uint32_t>(lights.size()) };
        light_buffer[frame_index].set_value(&light_header, sizeof(LightHeader));
        if (lights.size() > 0) { // update light buffer
            light_buffer[frame_index].set_value(lights.data(), sizeof(Light) * lights.size(), sizeof(LightHeader));
        }
    }
    
    { // record passes
        if (depth_pass.enabled() && depth_pass.version != current_version)
        {
            depth_pass.record(frame_index);
        }

        if (deferred_pass.enabled() && deferred_pass.version != current_version)
        {
            deferred_pass.record(frame_index);
        }

        if (culling_pass.enabled() && culling_pass.version != current_version)
        {
            culling_pass.record(frame_index);
        }

        if (forward_pass.enabled() && forward_pass.version != current_version)
        {
            forward_pass.record(image_index, frame_index);
        }
    }

    VK_ASSERT(vkResetFences(engine.device, 1, &in_flight[frame_index]));
    
    { // submit passes
        if (depth_pass.enabled())
        {
            depth_pass.submit(frame_index);
        }
        
        if (deferred_pass.enabled())
        {
            deferred_pass.submit(frame_index);
        }
        
        if (culling_pass.enabled())
        {
            culling_pass.submit(frame_index);
        }
        
        if (forward_pass.enabled())
        {
            forward_pass.submit(image_index, frame_index);
        }
    }

    { // present to screen
        VkPresentInfoKHR info{ 
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr, 
            1, &frame_ready[frame_index], // wait on frame ready
            1, &swapchain.swapchain, &image_index, 
            nullptr
        };
        VkResult res = vkQueuePresentKHR(engine.present.queue, &info);

        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            return;
        } else {
            VK_ASSERT(res);
        }
    }
    
    frame_index = (frame_index + 1) % settings.frame_count; // increment frame
}
*/