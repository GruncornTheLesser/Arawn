#pragma once
#include "vulkan.h"
#include "attachments.h"
#include "pass/depth.h"
#include "pass/deferred.h"
#include "pass/culling.h"
#include "pass/forward.h"

class Renderer {
    friend class DepthPass;
    friend class DeferredPass;
    friend class CullingPass;
    friend class ForwardPass;
public:
    Renderer();
    ~Renderer();
    Renderer(Renderer&&);
    Renderer& operator=(Renderer&&);
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void draw();

    void recreate();

    void record(uint32_t frame_index);

private:
    TextureAttachment msaa_attachment;
    TextureAttachment depth_attachment;
    TextureAttachment albedo_attachment;
    TextureAttachment normal_attachment;
    TextureAttachment position_attachment;

    BufferAttachment cluster_buffer; 
    BufferAttachment light_buffer;

    uint32_t frame_index = 0;
    uint32_t frame_count = 0;

    std::array<VK_TYPE(VkDescriptorSet), MAX_FRAMES_IN_FLIGHT> attachment_set;
    std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> image_available;
    std::array<VK_TYPE(VkFence), MAX_FRAMES_IN_FLIGHT> in_flight;

    DepthPass depth_pass;
    DeferredPass deferred_pass;
    CullingPass culling_pass;
    ForwardPass forward_pass;
};

extern Renderer renderer;