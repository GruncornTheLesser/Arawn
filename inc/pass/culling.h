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

    void record(uint32_t frame_index);
    bool enabled() { return cmd_buffer[0] != nullptr; }
    
    std::array<VK_TYPE(VkCommandBuffer), MAX_FRAMES_IN_FLIGHT> cmd_buffer;
    VK_TYPE(VkPipeline) pipeline;
    VK_TYPE(VkPipelineLayout) layout;
};