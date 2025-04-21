#pragma once
#include "vulkan.h"
#include "uniform.h"
#include "pass/depth.h"
#include "pass/deferred.h"
#include "pass/culling.h"
#include "pass/forward.h"

struct Frustum {
    glm::vec4 planes[4];
};

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
    Configuration config;

    glm::uvec3 cluster_count;
    uint32_t frame_index = 0;
    uint32_t frame_count = 0;

    std::array<Texture, MAX_FRAMES_IN_FLIGHT> msaa_attachment;      // msaa staging attachment
    std::array<Texture, MAX_FRAMES_IN_FLIGHT> depth_attachment;     // depth buffer attachment
    std::array<Texture, MAX_FRAMES_IN_FLIGHT> albedo_attachment;    // deferred colour attachment 1 (colour + alpha)
    std::array<Texture, MAX_FRAMES_IN_FLIGHT> normal_attachment;    // deferred colour attachment 2 (normal + metallic)
    std::array<Texture, MAX_FRAMES_IN_FLIGHT> position_attachment;  // deferred colour attachment 3 (normal + roughness)

    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> light_buffer;
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> cluster_buffer;
    Buffer frustum_buffer;

    std::array<VK_TYPE(VkFence), MAX_FRAMES_IN_FLIGHT> in_flight;
    std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT>     image_ready;
    std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT * 2> depth_ready;
    std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT>     light_ready;
    std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT>     defer_ready;
    std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT>     frame_ready;

    
    std::array<UniformSet, MAX_FRAMES_IN_FLIGHT> light_attachment_set;
    std::array<UniformSet, MAX_FRAMES_IN_FLIGHT> input_attachment_set;

    uint32_t current_version = 0;
    std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> frame_version;

    DepthPass depth_pass;
    DeferredPass deferred_pass;
    CullingPass culling_pass;
    ForwardPass forward_pass;
};

extern Renderer renderer;