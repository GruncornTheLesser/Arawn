#define ARAWN_INCLUDE_VULKAN
#include <engine/swap.h>
#include <algorithm>

using namespace arawn;

void engine::Swap::create(const Core& core, const Window& window, const Device& device, const Settings& info) {
	std::tie(format, colorspace) = [&]{
		uint32_t formatCount;
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(device.gpu, window.surface, &formatCount, nullptr));
		std::vector<VkSurfaceFormatKHR> supported(formatCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(device.gpu, window.surface, &formatCount, supported.data()));
		
		for (const auto& candidate : supported) { // if preferred format
			switch (candidate.format)
			{
				case(VK_FORMAT_R8G8B8A8_SRGB): break;
				case(VK_FORMAT_B8G8R8A8_SRGB): break;
				case(VK_FORMAT_R8G8B8A8_UNORM): break;
				case(VK_FORMAT_B8G8R8A8_UNORM): break;
				default: continue;
			}
		
			return std::pair{ candidate.format, candidate.colorSpace };
		}

		auto& candidate = supported.front();
		return std::pair{ candidate.format, candidate.colorSpace };
	}();

	// get surface capabilities
	VkSurfaceCapabilitiesKHR capabilities;
	VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.gpu, window.surface, &capabilities));
	
	// create swapchain
	std::array requiredFamilies = std::to_array({ 
		device.queue.graphics.family, 
		device.queue.present.family,
	});
	std::ranges::sort(requiredFamilies);
	std::span<uint32_t> families = { 
		requiredFamilies.data(),
		static_cast<uint32_t>(std::ranges::unique(requiredFamilies).begin() - requiredFamilies.begin())
	};
	VkSharingMode sharingMode = families.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	VkSwapchainCreateInfoKHR chainInfo{ 
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = window.surface,
		.minImageCount = chainInfo.minImageCount = [&]{
			uint32_t count = std::max(info.buffering == BufferingMode::TRIPLE ? 3u : 2u, capabilities.minImageCount);
			if (capabilities.maxImageCount != 0) {
				count = std::min(count, capabilities.maxImageCount);
			}
			return count;
		}(),
		.imageFormat = format,
		.imageColorSpace = colorspace,
		.imageExtent = [&]{
			if (capabilities.currentExtent.width != UINT32_MAX) {
				return capabilities.currentExtent;
			} else {
				return VkExtent2D{
					std::clamp(info.resolution.x, capabilities.minImageExtent.width,  capabilities.maxImageExtent.width),
					std::clamp(info.resolution.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
				};
			}
		}(),
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = sharingMode,
		.queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
		.pQueueFamilyIndices = families.data(),
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = [&]->VkPresentModeKHR {
			uint32_t presentModeCount;
			VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(device.gpu, window.surface, &presentModeCount, nullptr));
			std::vector<VkPresentModeKHR> presentModes(presentModeCount);
			VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(device.gpu, window.surface, &presentModeCount, presentModes.data()));
			
			if (info.vsync == VsyncMode::ENABLED) {
				if (info.latency != LowLatencyMode::DISABLED && std::ranges::contains(presentModes, VK_PRESENT_MODE_MAILBOX_KHR)) {
					return VK_PRESENT_MODE_MAILBOX_KHR;
				} else {
					return VK_PRESENT_MODE_FIFO_KHR;
				}
			} else {
				if (info.latency == LowLatencyMode::ENABLED && std::ranges::contains(presentModes, VK_PRESENT_MODE_IMMEDIATE_KHR)) {
					return VK_PRESENT_MODE_IMMEDIATE_KHR;
				} else if (std::ranges::contains(presentModes, VK_PRESENT_MODE_FIFO_RELAXED_KHR)) {
					return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
				} else {
					return VK_PRESENT_MODE_FIFO_KHR;
				}
			}
		}(),
		.clipped = VK_TRUE,
		.oldSwapchain = chain, // this path is also used by recreate
	};

	ARAWN_ASSERT(((capabilities.supportedUsageFlags & chainInfo.imageUsage) == chainInfo.imageUsage), "swapchain image usage not supported");

	ARAWN_LOG(VERBOSE, std::format("swap extent = {{ {}, {} }}", chainInfo.imageExtent.width, chainInfo.imageExtent.height))
	ARAWN_LOG(VERBOSE, std::format("swap present mode={}", string_VkPresentModeKHR(chainInfo.presentMode)));
	ARAWN_LOG(VERBOSE, std::format("swap format={}", string_VkFormat(chainInfo.imageFormat)));
	ARAWN_LOG(VERBOSE, std::format("swap colorspace={}", string_VkColorSpaceKHR(chainInfo.imageColorSpace)));
	
	VK_ASSERT(vkCreateSwapchainKHR(device.device, &chainInfo, nullptr, &chain));

	uint32_t imageCount;
	VK_ASSERT(vkGetSwapchainImagesKHR(device.device, chain, &imageCount, nullptr));
	images = std::vector<VkImage>(imageCount);
	VK_ASSERT(vkGetSwapchainImagesKHR(device.device, chain, &imageCount, images.data()));
}

void engine::Swap::recreate(const Core& core, const Window& window, const Device& device, const Settings& info) {
	VkSwapchainKHR old = chain;
	create(core, window, device, info);
	vkDestroySwapchainKHR(device.device, old, nullptr);
}

void engine::Swap::destroy(const Core& core, const Device& device) {
	vkDestroySwapchainKHR(device.device, chain, nullptr);
}