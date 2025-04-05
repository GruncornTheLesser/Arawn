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

private:
    struct GraphicPipeline {
        ~GraphicPipeline();

        std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
        std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> finished;
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkRenderPass) renderpass;
        VK_TYPE(VkPipelineLayout) layout;
        
    };

    struct ComputePipeline {
        ~ComputePipeline();

        std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer{};
        std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> finished;
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkPipelineLayout) layout;
    };

    TextureAttachment msaa_attachment;      // enabled when anti alias not NONE
    TextureAttachment depth_attachment;     // enabled when depth mode not NONE
    TextureAttachment albedo_attachment;    // enabled when render mode DEFERRED
    TextureAttachment normal_attachment;    // enabled when render mode DEFERRED
    TextureAttachment specular_attachment;  // enabled when render mode DEFERRED


    // BufferAttachment light_buffer;
    // BufferAttachment cluster_buffer;

    uint32_t frame_index = 0;

    struct DepthPass : GraphicPipeline {
        std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;
        
        void record(uint32_t frame_index);
    } depth_pass;

    struct CullingPass : ComputePipeline { 
        void record(uint32_t frame_index);
    } culling_pass;

    struct DeferredPass : GraphicPipeline { 
        std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;
        std::array<VK_TYPE(VkDescriptorSet), MAX_FRAMES_IN_FLIGHT> attachment_set;
        void record(uint32_t frame_index);
    } deferred_pass;

    struct ForwardPass : GraphicPipeline { 
        std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> image_available;
        std::array<VK_TYPE(VkFence), MAX_FRAMES_IN_FLIGHT> in_flight;
        std::vector<VK_TYPE(VkFramebuffer)> framebuffer;
        
        void record(uint32_t frame_index);
    } forward_pass;
};

extern Renderer renderer;