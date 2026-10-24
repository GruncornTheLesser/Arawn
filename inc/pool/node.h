#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <common/handle.h>
#include <common/pool.h>

namespace arawn::pool {
    struct Node {
        void create(const core::Device& device, const core::Settings& settings);
        void destroy(const core::Device& device);

        Handle<node::Node> create_node(const node::Node& createInfo);
        void destroy_node(Handle<node::Node> hnd);

        common::Pool pool;

        VK_TYPE(VkBuffer) buffer;
        VK_TYPE(VmaAllocation) memory;
        primitive::Node* mapped;
    };
}
