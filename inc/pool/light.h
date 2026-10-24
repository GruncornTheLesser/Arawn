#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <common/pool.h>

namespace arawn::pool {
    struct Light {
        void create(const core::Device& device, const core::Settings& settings);
        void destroy(const core::Device& device);

        Handle<node::Light> create_light(const node::Light& createInfo);
        void destroy_light(Handle<node::Light> hnd);

        common::Pool pool;

        VK_TYPE(VkBuffer) buffer;
        VK_TYPE(VmaAllocation) memory;
        primitive::Light* mapped;
    };
}
