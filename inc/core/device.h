#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::core {
	struct Device {
		void create(const Instance& instance, const Window& window, const Settings& info);
		void recreate(const Instance& instance, const Window& window, const Settings& info);
		void destroy(const Instance& instance);

		operator VK_TYPE(VkDevice)() const;

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
	};
}