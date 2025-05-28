#pragma once
#include "vulkan.h"
#include "settings.h"
#include "uniform.h"

class Renderer;

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