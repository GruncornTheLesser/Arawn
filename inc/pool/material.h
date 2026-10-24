#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <common/pool.h>
#include <primitive/material.h>
#include <asset/material.h>

namespace arawn::pool {
    struct Material {
        void create(const core::Device& device, const core::Settings& settings);
        void destroy(const core::Device& device);

        Handle<asset::Material> create_material(const asset::Material& createInfo);
        void destroy_material(Handle<asset::Material> hnd);

        common::Pool pool;

        VK_TYPE(VkBuffer) buffer;
        VK_TYPE(VmaAllocation) memory;
        primitive::Material* mapped;
    };
}
