#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <common/pool.h>

namespace arawn::pool {
    struct Light : private common::Pool {
        void create(const core::Device& device, const core::Settings& settings);
        void destroy(const core::Device& device);

        Handle<node::Light> create_light(const node::Light& createInfo);
        void destroy_light(Handle<node::Light> hnd);

        void flush();
        void flush_all();

        VK_TYPE(VkBuffer) buffer;
        VK_TYPE(VmaAllocation) memory;
        node::Light* mapped;
    };
}
