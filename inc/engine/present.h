#pragma once
#include "device.h"
#include <vector>

namespace arawn::engine {
	struct Present {
		struct Frame {
			VK_TYPE(VkCommandBuffer) cmd;
		};
		void create(const Window& window, const Device& device, const Render& render, const Settings& info);
		void recreate(const Window& window, const Device& device, const Render& render, const Settings& info);
		void destroy(const Device& device);

		std::vector<Frame> frames;
		uint32_t index;
	};
}