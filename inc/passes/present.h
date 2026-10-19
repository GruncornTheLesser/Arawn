#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
// #include <attachments/depth.h>

namespace arawn::passes {

    struct Present {
        void create();
        bool recreate();
        void destroy();

        VK_TYPE(VkCommandBuffer) cmds[ARAWN_MAX_FRAMES * ARAWN_MAX_IMAGES];
        VK_TYPE(VkSemaphore) finished[ARAWN_MAX_FRAMES];  
    };
}