#pragma once
#include <assets/mesh.h>
#include <assets/texture.h>
#include <engine/device.h>
#include <resources/buffer.h>
#include <resources/image.h>

namespace arawn::core {
	struct World {
		void create(const Device& device, const Settings& info);
		void recreate(const Device& device, const Settings& info);
		void destroy(const Device& device);

		struct {
			struct : resources::Buffer {
				VK_TYPE(VmaVirtualBlock) block;
			} arena; // block allocation of mesh data

			struct : resources::Buffer { } pool; // mesh manifest
		} mesh;

		struct : resources::Image {  // mega texture
			VK_TYPE(VkSampler) sampler;
		} texture;
		resources::Buffer material;
		resources::Buffer instance;
		resources::Buffer light;
		
		struct { 
			VK_TYPE(VkDescriptorSetLayout) layout;
			VK_TYPE(VkDescriptorSet) set;
		} descriptor; // descriptor for world data
	};
}