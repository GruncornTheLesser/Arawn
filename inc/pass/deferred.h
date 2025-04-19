#pragma once
#include "vulkan.h"

class Renderer;

class DeferredPass { 
public:
    DeferredPass() { cmd_buffer[0] = nullptr; }
    DeferredPass(Renderer& renderer);
    ~DeferredPass();
    DeferredPass(DeferredPass&& other);
    DeferredPass& operator=(DeferredPass&& other);
    DeferredPass(const DeferredPass& other) = delete;
    DeferredPass& operator=(const DeferredPass& other) = delete;
    
    void record(uint32_t frame_index);
    bool enabled() { return cmd_buffer[0] != nullptr; }

    std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
    VK_TYPE(VkPipeline) pipeline;
    VK_TYPE(VkRenderPass) renderpass;
    VK_TYPE(VkPipelineLayout) layout;
    std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;
};