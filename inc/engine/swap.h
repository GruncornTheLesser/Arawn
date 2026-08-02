#pragma once
#include "device.h"
#include <vector>

namespace arawn::engine {
	struct Swap {
		void create(const Core& core, const Window& window, const Device& device, const Settings& info);
		void recreate(const Core& core, const Window& window, const Device& device, const Settings& info);
		void destroy(const Core& core, const Device& device);

		VK_ENUM(VkFormat) format;
		VK_ENUM(VkColorSpaceKHR) colorspace;
		VK_TYPE(VkSwapchainKHR) chain = nullptr;
		std::vector<VK_TYPE(VkImage)> images;
	};
}