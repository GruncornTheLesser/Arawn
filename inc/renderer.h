#pragma once
#include "vulkan.h"
#include "uniform.h"
#include "attachments.h"
#include "pass/depth.h"
#include "pass/deferred.h"
#include "pass/culling.h"
#include "pass/forward.h"

struct Frustrum {
    glm::vec4 planes[4];
};

struct Cluster { // axis aligned bounding box
	glm::vec4 min_position, max_position;
};

template<size_t N>
struct LightArray {
    uint32_t count;
    int padding[3];
    uint32_t data[N];
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
    glm::uvec3 cell_count; // resolution / tile_size + 1

    TextureAttachment msaa_attachment;      // msaa staging attachment
    TextureAttachment depth_attachment;     // depth buffer attachment
    TextureAttachment albedo_attachment;    // deferred colour attachment 1 (colour + alpha)
    TextureAttachment normal_attachment;    // deferred colour attachment 2 (normal + metallic)
    TextureAttachment position_attachment;  // deferred colour attachment 3 (normal + roughness)

    BufferAttachment cluster_buffer; // cluster/frustrum buffer
    BufferAttachment culled_buffer;   // { index, count }
    BufferAttachment light_buffer;   // uniform buffer

    uint32_t frame_index = 0;
    uint32_t frame_count = 0;
    
    uint32_t current_version = 0;
    std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> frame_version;

    std::array<VK_TYPE(VkFence), MAX_FRAMES_IN_FLIGHT> in_flight;
    std::array<VK_TYPE(VkSemaphore), MAX_FRAMES_IN_FLIGHT> image_available;
    
    std::array<VK_TYPE(VkDescriptorSet), MAX_FRAMES_IN_FLIGHT> light_attachment_set;
    std::array<VK_TYPE(VkDescriptorSet), MAX_FRAMES_IN_FLIGHT> input_attachment_set;

    DepthPass depth_pass;
    DeferredPass deferred_pass;
    CullingPass culling_pass;
    ForwardPass forward_pass;
};

extern Renderer renderer;