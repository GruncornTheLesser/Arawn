#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
// #include <depth.h>

namespace arawn::pass {

    struct LateCull {
        void create(const core::Device& device);
        bool recreate();
        void destroy(const core::Device& device);

        VK_TYPE(VkCommandBuffer) cmds[ARAWN_MAX_FRAMES];
        VK_TYPE(VkSemaphore) finished[ARAWN_MAX_FRAMES];

        
    };


}