#pragma once
#include "vulkan.h"
#include "uniform.h"
#include <span>
#include "attachments.h"

// TODO: work out padding/packing

struct Light {
    glm::vec3 position;
    float radius;
    glm::vec3 intensity;
private:
    float padding;
};

struct Frustrum {
    glm::vec3 bb_min; // bounding box min 
    int light_offset;
    glm::vec3 bb_max; // bounding box max
    uint32_t light_count;
};

struct PushConstants {
    glm::ivec2 viewport_size;
    glm::ivec3 cluster_count;
    int debug_mode;
};

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
    TextureAttachment msaa_attachment;      // enabled when anti alias not NONE
    TextureAttachment depth_attachment;     // enabled when depth mode not NONE
    TextureAttachment albedo_attachment;    // enabled when render mode DEFERRED
    TextureAttachment normal_attachment;    // enabled when render mode DEFERRED
    TextureAttachment specular_attachment;  // enabled when render mode DEFERRED


    // BufferAttachment light_buffer;
    // BufferAttachment cluster_buffer;

    uint32_t frame_index = 0;
    uint32_t frame_count = 0;

    struct DepthPass {
        DepthPass() { cmd_buffer[0] = nullptr; }
        DepthPass(Renderer& renderer);
        ~DepthPass();
        DepthPass(DepthPass&& other);
        DepthPass& operator=(DepthPass&& other);
        DepthPass(const DepthPass& other) = delete;
        DepthPass& operator=(const DepthPass& other) = delete;

        void record(uint32_t frame_index);

        std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
        std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> finished;
        
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkRenderPass) renderpass;
        VK_TYPE(VkPipelineLayout) layout;

        std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;


    } depth_pass;

    struct CullingPass { 
        CullingPass() { cmd_buffer[0] = nullptr; }
        CullingPass(Renderer& renderer);
        ~CullingPass();
        CullingPass(CullingPass&& other);
        CullingPass& operator=(CullingPass&& other);
        CullingPass(const CullingPass& other) = delete;
        CullingPass& operator=(const CullingPass& other) = delete;

        void record(uint32_t frame_index);
        
        std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
        std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> finished;
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkRenderPass) renderpass;
        VK_TYPE(VkPipelineLayout) layout;
    } culling_pass;

    struct DeferredPass { 
        DeferredPass() { cmd_buffer[0] = nullptr; }
        DeferredPass(Renderer& renderer);
        ~DeferredPass();
        DeferredPass(DeferredPass&& other);
        DeferredPass& operator=(DeferredPass&& other);
        DeferredPass(const DeferredPass& other) = delete;
        DeferredPass& operator=(const DeferredPass& other) = delete;
        
        void record(uint32_t frame_index);

        std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
        std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> finished;
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkRenderPass) renderpass;
        VK_TYPE(VkPipelineLayout) layout;
        std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;
    } deferred_pass;

    struct ForwardPass { 
        ForwardPass() { cmd_buffer[0] = nullptr; }
        ForwardPass(Renderer& renderer);
        ~ForwardPass();
        ForwardPass(ForwardPass&& other);
        ForwardPass& operator=(ForwardPass&& other);
        ForwardPass(const ForwardPass& other) = delete;
        ForwardPass& operator=(const ForwardPass& other) = delete;

        void record(uint32_t frame_index);

        std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
        std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> finished;
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkRenderPass) renderpass;
        VK_TYPE(VkPipelineLayout) layout;
        std::vector<VK_TYPE(VkFramebuffer)> framebuffer;
    } forward_pass;

    std::array<VK_TYPE(VkDescriptorSet), MAX_FRAMES_IN_FLIGHT> attachment_set;
    std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> image_available;
    std::array<VK_TYPE(VkFence), MAX_FRAMES_IN_FLIGHT> in_flight;
};

extern Renderer renderer;