#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::pool {
    struct Texture {
        void create(const core::Device& device, const core::Settings& settings);
        void destroy(const core::Device& device);
    };
}
