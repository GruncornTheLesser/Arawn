#pragma once
#include "vulkan.h"

class Renderer;

class ForwardPass {
public:
    ForwardPass() { cmd_buffer[0] = nullptr; }
    ForwardPass(Renderer& renderer);
    ~ForwardPass();
    ForwardPass(ForwardPass&& other);
    ForwardPass& operator=(ForwardPass&& other);
    ForwardPass(const ForwardPass& other) = delete;
    ForwardPass& operator=(const ForwardPass& other) = delete;

    void record(uint32_t frame_index);
    bool enabled() { return cmd_buffer[0] != nullptr; }

    std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
    VK_TYPE(VkPipeline) pipeline;
    VK_TYPE(VkRenderPass) renderpass;
    VK_TYPE(VkPipelineLayout) layout;
    std::vector<VK_TYPE(VkFramebuffer)> framebuffer;
};