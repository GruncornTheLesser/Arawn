#pragma once
#include "device.h"
#include <vector>

namespace arawn::engine {
	struct Present {
		struct Frame { 
			// TODO:
		};
		void create(const Core& core, const Window& window, const Device& device, const Settings& info);
		void recreate(const Core& core, const Window& window, const Device& device, const Settings& info);
		void destroy(const Core& core, const Device& device);

		std::vector<Frame> frames;
		uint32_t index;
	};
}