#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::passes {

    struct Forward {
        void create();
        bool recreate();
        void destroy();

        VK_TYPE(VkCommandBuffer) cmds[ARAWN_MAX_FRAMES];
        VK_TYPE(VkSemaphore) finished[ARAWN_MAX_FRAMES];

        
    };


}