#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <common/pool.h>

namespace arawn::pool {
    struct Mesh {
        void create(const core::Device& device, const core::Settings& settings);
        void destroy(const core::Device& device);

        Handle<asset::Mesh> create_mesh(const asset::Mesh& createInfo);
        void destroy_mesh(Handle<asset::Mesh> hnd);

        common::Pool pool;

        VK_TYPE(VkBuffer) buffer;
        VK_TYPE(VmaAllocation) memory;
        primitive::Material* mapped;
    };
}
