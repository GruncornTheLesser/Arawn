#define ARAWN_INCLUDE_VULKAN
#include <core/swap.h>
#include <core/device.h>
#include <core/window.h>
#include <core/settings.h>
#include <core/frame.h>
#include <algorithm>

using namespace arawn;
using namespace arawn::core;

void Swap::create(const Window& window, const Device& device, const Settings& info) {
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
	std::array requiredFamilies = std::to_array({ device.queue.graphics.family, device.queue.present.family });
	std::ranges::sort(requiredFamilies);
	std::span<uint32_t> families = { requiredFamilies.data(), static_cast<uint32_t>(std::ranges::unique(requiredFamilies).begin() - requiredFamilies.begin()) };
	VkSharingMode sharingMode = families.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

	
	// get min image count
	uint32_t minImageCount = std::max(info.buffering == BufferingMode::TRIPLE ? 3u : 2u, capabilities.minImageCount);
	if (capabilities.maxImageCount != 0) {
		minImageCount = std::min(minImageCount, capabilities.maxImageCount);
	}

	// get extent
	VkExtent2D extent;
	if (capabilities.currentExtent.width != UINT32_MAX) {
		extent = capabilities.currentExtent;
	} else {
		extent = {
			std::clamp(info.resolution.x, capabilities.minImageExtent.width,  capabilities.maxImageExtent.width),
			std::clamp(info.resolution.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

	VkPresentModeKHR presentMode;
	
	uint32_t presentModeCount;
	VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(device.gpu, window.surface, &presentModeCount, nullptr));
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(device.gpu, window.surface, &presentModeCount, presentModes.data()));
	
	if (info.vsync == VsyncMode::ENABLED) {
		if (info.latency != LowLatencyMode::DISABLED && std::ranges::contains(presentModes, VK_PRESENT_MODE_MAILBOX_KHR)) {
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		} else {
			presentMode = VK_PRESENT_MODE_FIFO_KHR;
		}
	} else {
		if (info.latency == LowLatencyMode::ENABLED && std::ranges::contains(presentModes, VK_PRESENT_MODE_IMMEDIATE_KHR)) {
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		} else if (std::ranges::contains(presentModes, VK_PRESENT_MODE_FIFO_RELAXED_KHR)) {
			presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
		} else {
			presentMode = VK_PRESENT_MODE_FIFO_KHR;
		}
	}


	VkSwapchainCreateInfoKHR chainInfo{ 
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = window.surface,
		.minImageCount = minImageCount,
		.imageFormat = format,
		.imageColorSpace = colorspace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = sharingMode,
		.queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
		.pQueueFamilyIndices = families.data(),
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = chain, // this path is also used by recreate
	};

	ARAWN_ASSERT(((capabilities.supportedUsageFlags & chainInfo.imageUsage) == chainInfo.imageUsage), "swapchain image usage not supported");

	ARAWN_LOG(VERBOSE, std::format("swap extent = {{ {}, {} }}", chainInfo.imageExtent.width, chainInfo.imageExtent.height));
	ARAWN_LOG(VERBOSE, std::format("swap present mode={}", string_VkPresentModeKHR(chainInfo.presentMode)));
	ARAWN_LOG(VERBOSE, std::format("swap format={}", string_VkFormat(chainInfo.imageFormat)));
	ARAWN_LOG(VERBOSE, std::format("swap colorspace={}", string_VkColorSpaceKHR(chainInfo.imageColorSpace)));
	
	VK_ASSERT(vkCreateSwapchainKHR(device.device, &chainInfo, nullptr, &chain));

	uint32_t imageCount;
	VK_ASSERT(vkGetSwapchainImagesKHR(device.device, chain, &imageCount, nullptr));
	images = std::vector<VkImage>(imageCount);
	VK_ASSERT(vkGetSwapchainImagesKHR(device.device, chain, &imageCount, images.data()));
}

/*
void Swap::recreate(const Window& window, const Device& device, const Settings& info) {
	VkSwapchainKHR old = chain;
	create(window, device, info);
	vkDestroySwapchainKHR(device.device, old, nullptr);
}
*/

void Swap::destroy(const Device& device) {
	vkDestroySwapchainKHR(device.device, chain, nullptr);
}

VkResult Swap::advance(const Device &device, const core::Frame& frame) {
	const uint64_t timeout = 1000000000;
	VK_ASSERT(vkWaitForFences(device.device, 1, &frame.inFlight[frame.index], true, timeout));
	
	uint32_t swapIndex;
	return vkAcquireNextImageKHR(device.device, chain, timeout, frame.imageReady[frame.index], nullptr, &swapIndex);
}