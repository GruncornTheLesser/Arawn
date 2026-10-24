#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::pass {

    struct Forward {
        void create(const core::Device& device);
        void destroy(const core::Device& device);

        VK_TYPE(VkCommandBuffer) cmds[ARAWN_MAX_FRAMES];
        VK_TYPE(VkSemaphore) finished[ARAWN_MAX_FRAMES];
    };


}