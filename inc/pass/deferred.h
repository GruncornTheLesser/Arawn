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