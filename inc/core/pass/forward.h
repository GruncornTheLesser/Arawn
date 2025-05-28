#pragma once
#include "vulkan.h"

class Renderer;

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

    struct PresentData {
        VK_TYPE(VkImageView) view;
        std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
        std::array<VK_TYPE(VkFramebuffer), MAX_FRAMES_IN_FLIGHT> framebuffer;
        std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> version;
    };

    std::vector<PresentData> data;
};