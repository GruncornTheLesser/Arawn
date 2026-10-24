#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <common/pool.h>
#include <asset/material.h>

namespace arawn::pool {
    struct Material : common::Pool {
        void create(const core::Device& device, const core::Settings& settings);
        void destroy(const core::Device& device);

        Handle<asset::Material> create_material(const asset::Material& createInfo);
        void destroy_material(Handle<asset::Material> hnd);

        void flush();
        void flush_all();

        VK_TYPE(VkBuffer) buffer;
        VK_TYPE(VmaAllocation) memory;
        asset::Material* mapped;
    };
}
