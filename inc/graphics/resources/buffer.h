#pragma once
#include <graphics/engine.h>

namespace Arawn {
	struct Buffer { 
		friend struct Graph;
		struct Usage { 
			VK_ENUM(VkAccessFlags) access;
			VK_ENUM(VkPipelineStageFlags) stages;
			VK_ENUM(VkAttachmentLoadOp) loadop;
		};
		Buffer(std::size_t size, VK_ENUM(VkBufferUsageFlags) usage, VK_ENUM(VmaMemoryUsage) mem_usage);
		~Buffer();
	
		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;
	
		Buffer(Buffer&& other) noexcept;
		Buffer& operator=(Buffer&& other) noexcept;
	private:
		VK_TYPE(VkBuffer) buffer;
		VK_TYPE(VmaAllocation) memory;
	};
}