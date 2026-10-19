#pragma once
#include <common/vulkan.h>
// #include <attachments/depth.h>

namespace arawn::passes {

    struct DepthOnly {
        void create();
        bool recreate();
        void destroy();

        VK_TYPE(VkCommandBuffer) cmds[ARAWN_MAX_FRAMES];
        VK_TYPE(VkSemaphore) finished[ARAWN_MAX_FRAMES];
    };


}