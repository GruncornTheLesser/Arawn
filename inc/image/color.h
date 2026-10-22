#pragma once 
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::image {
    struct Color {
        void create(const core::Device& device, const core::Frame& frame, const core::Settings& settings);
        void recreate(const core::Device& device, const core::Frame& frame, const core::Settings& settings);
        void destroy(const core::Device& device);

        VK_TYPE(VkImage) image[ARAWN_MAX_FRAMES];
        VK_TYPE(VmaAllocation) memory[ARAWN_MAX_FRAMES];
    };
}