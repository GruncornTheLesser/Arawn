#define ARAWN_IMPLEMENTATION
#include <graphics/resources/image.h>

Arawn::Image::Image(VK_ENUM(VkFormat) format, uint32_t width, uint32_t height, VK_ENUM(VkImageUsageFlags) usage) {
	VkImageCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent  = { width, height, 1 },
		.mipLevels = 1,
		.arrayLayers = 1, 
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, 
	};

	VmaAllocationCreateInfo alloc {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY
	};

	vmaCreateImage(engine.allocator, &info, &alloc, &image, &memory, nullptr);

	VkImageViewCreateInfo viewInfo {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, 
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};

	vkCreateImageView(engine.device, &viewInfo, nullptr, &view);

}
Arawn::Image::~Image() {
	if (image == nullptr) {
		vkDestroyImageView(engine.device, view, nullptr);
		vmaDestroyImage(engine.allocator, image, memory);
	}
}

Arawn::Image::Image(Image&& other) noexcept {
	image = other.image;
	memory = other.memory;
	view = other.view;

	image = nullptr;
}
Arawn::Image& Arawn::Image::operator=(Image&& other) noexcept {
	if (image == nullptr) {
		vkDestroyImageView(engine.device, view, nullptr);
		vmaDestroyImage(engine.allocator, image, memory);
	}
	
	image = other.image;
	memory = other.memory;
	view = other.view;

	image = nullptr;

	return *this;
}
