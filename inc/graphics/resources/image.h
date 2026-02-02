#pragma once
#include <graphics/engine.h>

namespace Arawn {
	struct Image { 
		friend struct Graph;
		struct Usage { 
			VK_ENUM(VkImageLayout) layout;
			VK_ENUM(VkAccessFlags) access;
			VK_ENUM(VkPipelineStageFlags) stages;
			VK_ENUM(VkAttachmentLoadOp) loadop;
		};
		Image(VK_ENUM(VkFormat) format, uint32_t width, uint32_t height, VK_ENUM(VkImageUsageFlags) usage);
		~Image();
	
		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;
		Image(Image&& other) noexcept;
		Image& operator=(Image&& other) noexcept;
	private:
		VK_TYPE(VkImage) image;
		VK_TYPE(VkImageView) view;
		VK_TYPE(VmaAllocation) memory;
	};
}