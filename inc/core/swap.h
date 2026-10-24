#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <vector>

namespace arawn::core {
	struct Swap {
		void create(const Window& window, const Device& device, const Settings& info);
		void destroy(const Device& device);
		
		VK_ENUM(VkResult) advance(const Device& device, const core::Frame& frame);

		operator VK_TYPE(VkSwapchainKHR)() const;

		VK_ENUM(VkFormat) format;
		VK_ENUM(VkColorSpaceKHR) colorspace;
		VK_TYPE(VkSwapchainKHR) chain = nullptr;
		std::vector<VK_TYPE(VkImage)> images;
		uint32_t index;
	};
}