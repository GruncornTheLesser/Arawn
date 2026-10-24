#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <common/pool.h>

namespace arawn::pool {
    struct Mesh {
        void create(const core::Device& device, const core::Settings& settings);
        void destroy(const core::Device& device);

        Handle<asset::Mesh> create_mesh(const std::vector<primitive::Vertex>& vertices, const std::vector<uint32_t>& indices);
        void destroy_mesh(Handle<asset::Mesh> hnd);
        
        void update(const core::Frame& frame);

        struct Allocation {
            struct {
                VK_TYPE(VmaVirtualAllocation) memory;
            } vertex, index;
        };

        common::Pool pool;

        struct {
            VK_TYPE(VkBuffer) buffer;
            VK_TYPE(VmaAllocation) memory;
            VK_TYPE(VmaVirtualBlock) block;
        } arena;

        struct {
            VK_TYPE(VkBuffer) buffer;
            VK_TYPE(VmaAllocation) memory;
            asset::Mesh* mapped;
        } manifest; // packed container of current meshes
    };
}
