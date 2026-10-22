#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <span>

namespace arawn::core {
	struct Device {
		void create(const Instance& instance, const Window& window, const Settings& info);
		void recreate(const Instance& instance, const Window& window, const Settings& info);
		void destroy(const Instance& instance);

		operator VK_TYPE(VkDevice)() const;

		VK_ENUM(VkFormat) selectFormat(std::span<const VK_ENUM(VkFormat)> candidates, VK_ENUM(VkImageTiling) tiling, VK_ENUM(VkFormatFeatureFlags) features);

		VK_TYPE(VkPhysicalDevice) gpu;
		VK_TYPE(VkDevice) device;
		
		struct Queue {
			uint32_t family, index; 
			VK_TYPE(VkQueue) queue;
			VK_TYPE(VkCommandPool) pool;
		};
		struct { 
			Queue graphics;
			Queue compute;
			Queue transfer;
			Queue present;
		} queue;

		VK_TYPE(VmaAllocator) allocator;

		struct {
			VK_ENUM(VkFormat) depth;
			VK_ENUM(VkFormat) color;
			VK_ENUM(VkFormat) ztree;
		} format;
	};
}