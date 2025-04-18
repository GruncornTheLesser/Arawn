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

struct Cluster { // axis aligned bounding box
	glm::vec4 min_position, max_position;
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
    glm::uvec3 frustum_count; // resolution / tile_size + 1

    uint32_t frame_index = 0;
    uint32_t frame_count = 0;

    std::array<Texture, MAX_FRAMES_IN_FLIGHT> msaa_attachment;      // msaa staging attachment
    std::array<Texture, MAX_FRAMES_IN_FLIGHT> depth_attachment;     // depth buffer attachment
    std::array<Texture, MAX_FRAMES_IN_FLIGHT> albedo_attachment;    // deferred colour attachment 1 (colour + alpha)
    std::array<Texture, MAX_FRAMES_IN_FLIGHT> normal_attachment;    // deferred colour attachment 2 (normal + metallic)
    std::array<Texture, MAX_FRAMES_IN_FLIGHT> position_attachment;  // deferred colour attachment 3 (normal + roughness)

    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> frustum_buffer; // cluster/Frustum buffer
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> culled_buffer;   // { index, count }
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> light_buffer;   // uniform buffer
    
    std::array<UniformSet, MAX_FRAMES_IN_FLIGHT> light_attachment_set;
    std::array<UniformSet, MAX_FRAMES_IN_FLIGHT> input_attachment_set;

    uint32_t current_version = 0;
    std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> frame_version;

    std::array<VK_TYPE(VkFence), MAX_FRAMES_IN_FLIGHT> in_flight;
    std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> image_available;
    
    DepthPass depth_pass;
    DeferredPass deferred_pass;
    CullingPass culling_pass;
    ForwardPass forward_pass;
};

extern Renderer renderer;