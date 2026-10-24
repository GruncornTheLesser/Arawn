#pragma once 
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::attachment::cull {
    struct Light {
        void create(const core::Device& device, const core::Frame& frame, const core::Settings& settings);
        void destroy(const core::Device& device);

        struct {
            VK_TYPE(VkBuffer) buffer[ARAWN_MAX_FRAMES];
            VK_TYPE(VmaAllocation) memory[ARAWN_MAX_FRAMES];
        } mainifest; // lights drawn per frame

        struct {
            VK_TYPE(VkBuffer) buffer[ARAWN_MAX_FRAMES];
            VK_TYPE(VmaAllocation) memory[ARAWN_MAX_FRAMES];
        } tile; // tile + culled lights 
    };
}