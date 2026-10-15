#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <vector>

namespace arawn::engine {
	struct Swap {
		void create(const Window& window, const Device& device, const Settings& info);
		void recreate(const Window& window, const Device& device, const Settings& info);
		void destroy(const Device& device);

		VK_ENUM(VkFormat) format;
		VK_ENUM(VkColorSpaceKHR) colorspace;
		VK_TYPE(VkSwapchainKHR) chain = nullptr;
		std::vector<VK_TYPE(VkImage)> images;
		uint32_t index;
	};
}