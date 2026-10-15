#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <initializer_list>

namespace arawn::resources {
    struct Buffer {
		void create(
			const engine::Device& device,
			std::size_t size,
			VK_ENUM(VkBufferUsageFlags) usage,
			VK_ENUM(VmaAllocationCreateFlags) flags,
			std::initializer_list<uint32_t> queues,
			void** mapped = nullptr
		);
		void destroy(const engine::Device& device);
		
		VK_TYPE(VkBuffer) buffer;
		VK_TYPE(VmaAllocation) memory;
	};
}