#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>


namespace arawn::core {
    struct Frame {
		void create(const core::Device& device, const Settings& setting);
        void destroy(const core::Device& device);
        
        void advance();

        uint32_t count;
        uint32_t index;

        VK_TYPE(VkFence) inFlight[ARAWN_MAX_FRAMES];
        VK_TYPE(VkSemaphore) imageReady[ARAWN_MAX_FRAMES];
    };
}