#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <common/pool.h>

namespace arawn::pool {
    struct Light {
        void create(const core::Device& device, const core::Settings& settings);
        void recreate(const core::Device& device, const core::Settings& settings);
        void destroy(const core::Device& device);

    };
}
