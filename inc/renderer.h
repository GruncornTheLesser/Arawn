#pragma once
#include "vulkan.h"
#include <span>

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
    int light_count;
};

struct Camera {
    glm::mat4 projview;
    glm::vec3 position;
private:
    float padding;
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
    struct TextureAttachment {
        ~TextureAttachment();

        std::array<VK_TYPE(VkImage), MAX_FRAMES_IN_FLIGHT> image;
        std::array<VK_TYPE(VkImageView), MAX_FRAMES_IN_FLIGHT> view;
        std::array<VK_TYPE(VkDeviceMemory), MAX_FRAMES_IN_FLIGHT> memory;
    };

    struct BufferAttachment {
        ~BufferAttachment();
        
        std::array<VK_TYPE(VkBuffer), MAX_FRAMES_IN_FLIGHT> buffer;
        std::array<VK_TYPE(VkDeviceMemory), MAX_FRAMES_IN_FLIGHT> memory;
    };


    struct GraphicPipeline {
        ~GraphicPipeline();

        void bind(VK_TYPE(VkCommandBuffer) cmd_buffer);

        void submit(uint32_t frame_index, 
            std::span<VK_TYPE(VkSemaphore)> wait_semaphore = { }, 
            std::span<VK_ENUM(VkPipelineStageFlags)> wait_stages = { }, 
            VK_TYPE(VkFence) signal_fence = nullptr
        );

        std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
        std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> finished;
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkRenderPass) renderpass;
        VK_TYPE(VkPipelineLayout) layout;
        
    };

    struct ComputePipeline {
        ~ComputePipeline();

        void bind(VK_TYPE(VkCommandBuffer) cmd_buffer);

        void submit(uint32_t frame_index, 
            std::span<VK_TYPE(VkSemaphore)> wait_semaphore = { }, 
            std::span<VK_ENUM(VkPipelineStageFlags)> wait_stages = { }, 
            VK_TYPE(VkFence) signal_fence = nullptr
        );

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

    BufferAttachment light_buffer;
    BufferAttachment cluster_buffer;

    uint32_t frame_index = 0;
    uint32_t image_index;

    struct DepthPass : GraphicPipeline {
        std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;
        
        void record(uint32_t frame_index);
    } depth_pass;

    struct ClusterPass : ComputePipeline { 
        void record(uint32_t frame_index);
    } culling_pass;

    struct DeferredPass : GraphicPipeline { 
        std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;
        
        void record(uint32_t frame_index);
    } deferred_pass;

    struct LightingPass : GraphicPipeline { 
        std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> image_available;
        std::array<VK_TYPE(VkFence), MAX_FRAMES_IN_FLIGHT> in_flight;
        std::vector<VK_TYPE(VkFramebuffer)> framebuffer;
        
        void record(uint32_t frame_index);
    } lighting_pass;
};