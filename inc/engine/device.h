#pragma once
#include "core.h"
#include "window.h"
namespace arawn::engine {
	struct Device {
		struct Queue {
			uint32_t family, index; 
			VK_TYPE(VkQueue) queue;
			VK_TYPE(VkCommandPool) pool;
		};
		
		void create(const Core& core, const Window& window, const Settings& info);
		void recreate(const Core& core, const Window& window, const Settings& info);
		void destroy(const Core& core);

		VK_TYPE(VkPhysicalDevice) gpu;
		VK_TYPE(VkDevice) device;
		
		struct { 
			Queue graphics;
			Queue compute;
			Queue transfer;
			Queue present;
		} queue;

		VK_TYPE(VmaAllocator) allocator;
		
		struct {
			VK_TYPE(VkDescriptorPool) pool;
		} descriptor;
	};
}