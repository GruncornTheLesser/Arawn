#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::engine {
	struct Device {
		void create(const Core& core, const Window& window, const Settings& info);
		void recreate(const Core& core, const Window& window, const Settings& info);
		void destroy(const Core& core);

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
			VK_TYPE(VkDescriptorPool) pool;
			struct {
				VK_TYPE(VkDescriptorSetLayout) world;
				VK_TYPE(VkDescriptorSetLayout) frame;
			} layout;
		} descriptor;

		struct {
			struct {
				VK_ENUM(VkFormat) color;
				VK_ENUM(VkFormat) depth;
				VK_ENUM(VkFormat) zTree;
				// VK_ENUM(VkFormat) ssr;
				// VK_ENUM(VkFormat) ao;
			} attachment;

			struct {
				VK_ENUM(VkFormat) albedo;
				VK_ENUM(VkFormat) normal;
				VK_ENUM(VkFormat) PBR;
				VK_ENUM(VkFormat) emissive;
			} material;

			struct {
				VK_ENUM(VkFormat) position;
				VK_ENUM(VkFormat) normal;
				VK_ENUM(VkFormat) tangent;
			} vertex;
		} format;
	};
}