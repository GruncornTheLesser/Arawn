#pragma once
#include "device.h"

namespace arawn::engine {
	struct World {
		void create(const Device& device, const Settings& info);
		void recreate(const Device& device, const Settings& info);
		void destroy(const Device& device);

		struct {
			struct {
				VK_TYPE(VkBuffer) buffer;
				VK_TYPE(VmaAllocation) memory;
				VK_TYPE(VmaVirtualBlock) block;
			} arena; // block allocation of mesh data

			VK_TYPE(VkBuffer) buffer; // ring buffer of current meshes
			VK_TYPE(VmaAllocation) memory;
		} mesh;
		struct {
			VK_TYPE(VkBuffer) buffer;
			VK_TYPE(VmaAllocation) memory;
			struct {  // mega texture
				VK_TYPE(VkImage) image;
				VK_TYPE(VmaAllocation) memory;
				VK_TYPE(VkSampler) sampler;
			} texture;
		} material;
		struct { 
			VK_TYPE(VkBuffer) buffer;
			VK_TYPE(VmaAllocation) memory;
		} instance;
		struct { 
			VK_TYPE(VkBuffer) buffer;
			VK_TYPE(VmaAllocation) memory;
		} light;
		struct { 
			VK_TYPE(VkDescriptorSetLayout) layout;
			VK_TYPE(VkDescriptorSet) set;
		} descriptor; // descriptor for mesh data
	};
}