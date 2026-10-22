#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>


namespace arawn::core {
    struct Frame {
		void create(const Settings& setting);
        bool query(const Settings& settings);
		void recreate(const Settings& setting);
        
        void advance();

        uint32_t count;
        uint32_t index;

        VK_TYPE(VkFence) inFlight[ARAWN_MAX_FRAMES];
        VK_TYPE(VkFence) imageReady[ARAWN_MAX_FRAMES];
        VK_TYPE(VkFence) frameReady[ARAWN_MAX_FRAMES];
    };
}