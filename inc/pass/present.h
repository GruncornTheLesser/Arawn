#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
// #include <depth.h>

namespace arawn::pass {

    struct Present {
        void create(const core::Device& device);
        void destroy(const core::Device& device);

        VK_TYPE(VkCommandBuffer) cmds[ARAWN_MAX_FRAMES * ARAWN_MAX_IMAGES];
        VK_TYPE(VkSemaphore) finished[ARAWN_MAX_FRAMES];  
    };
}