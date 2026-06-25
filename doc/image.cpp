#define ARAWN_IMPLEMENTATION

/*
#include <render/core/engine.h>
#include <render/resources/image.h>
VkFormat Arawn::Image::select_format(Format format, uint32_t mode, VkImageUsageFlags usage) {
    using namespace Arawn;
	
	VkFormatFeatureFlags features = 0;
	{ // get format features
		if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) { 
			features |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
			features |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
		}
		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) { 
			features |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
		}
    	if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) { 
			features |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
		}
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) { 
			features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		if (usage & VK_IMAGE_USAGE_STORAGE_BIT) { 
			features |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
		}
		if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) { 
			features |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
			features |= VK_FORMAT_FEATURE_BLIT_SRC_BIT;
		}
    	if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) { 
			features |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
			features |= VK_FORMAT_FEATURE_BLIT_DST_BIT;
		}
	}

	std::vector<VkFormat> candidates;
	switch (format.components) {
		case Format::R: {
			switch (format.precision) {
				case Format::HIGH_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R32_SFLOAT, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R8_UNORM }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R32_UINT, VK_FORMAT_R16_UINT, VK_FORMAT_R8_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R32_SINT, VK_FORMAT_R16_SINT, VK_FORMAT_R8_SINT }; break; }
					}
					break;
				}
				case Format::MEDIUM_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R8_UNORM }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R16_UINT, VK_FORMAT_R32_UINT, VK_FORMAT_R8_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R16_SINT, VK_FORMAT_R32_SINT, VK_FORMAT_R8_SINT }; break; }
					}
					break;
				}
				case Format::LOW_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R8_UNORM, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R32_SFLOAT }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R8_UINT, VK_FORMAT_R16_UINT, VK_FORMAT_R32_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R8_SINT, VK_FORMAT_R16_SINT, VK_FORMAT_R32_SINT }; break; }
					}
					break;
				}	
			}
			break;
		}
		break;
		case Format::R | Format::G: {
			switch (format.precision) {
				case Format::HIGH_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R8G8_UNORM }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R32G32_UINT, VK_FORMAT_R16G16_UINT, VK_FORMAT_R8G8_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R32G32_SINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R8G8_SINT }; break; }
					}
					break;
				}
				case Format::MEDIUM_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R8G8_UNORM }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R16G16_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R8G8_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R16G16_SINT, VK_FORMAT_R32G32_SINT, VK_FORMAT_R8G8_SINT }; break; }
					}
					break;
				}
				case Format::LOW_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R8G8_UNORM, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R32G32_SFLOAT }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R8G8_UINT, VK_FORMAT_R16G16_UINT, VK_FORMAT_R32G32_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R8G8_SINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R32G32_SINT }; break; }
					}
					break;
				}
			}
			break;
		}
		break;

		case Format::R | Format::G | Format::B: {
			switch (format.precision) {
				case Format::HIGH_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R8G8B8_UNORM,  VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R16G16B16_UINT, VK_FORMAT_R8G8B8_UINT,       VK_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R8G8B8A8_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R32G32B32_SINT, VK_FORMAT_R16G16B16_SINT, VK_FORMAT_R8G8B8_SINT,       VK_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R8G8B8A8_SINT }; break; }
					}
					break;
				}
				case Format::MEDIUM_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R8G8B8_UNORM,  VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R16G16B16_UINT, VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R8G8B8_UINT,       VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R8G8B8A8_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R16G16B16_SINT, VK_FORMAT_R32G32B32_SINT, VK_FORMAT_R8G8B8_SINT,       VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R8G8B8A8_SINT }; break; }
					}
					break;
				}
				case Format::LOW_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT,  VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R8G8B8_UINT, VK_FORMAT_R16G16B16_UINT, VK_FORMAT_R32G32B32_UINT,       VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R32G32B32A32_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R8G8B8_SINT, VK_FORMAT_R16G16B16_SINT, VK_FORMAT_R32G32B32_SINT,       VK_FORMAT_R8G8B8A8_SINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R32G32B32A32_SINT }; break; }
					}
					break;
				}
			}
			break;
		}
		case Format::R | Format::G | Format::B | Format::A: {
			switch (format.precision) {
				case Format::HIGH_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R8G8B8A8_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R8G8B8A8_SINT }; break; }
					}
					break;
				}
				case Format::MEDIUM_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R8G8B8A8_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R8G8B8A8_SINT }; break; }
					}
					break;
				}
				case Format::LOW_PRECISION: {
					switch (format.type) {
						case Format::FLOAT: { candidates = { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT }; break; }
						case Format::UINT: { candidates =  { VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R32G32B32A32_UINT }; break; }
						case Format::INT: { candidates =   { VK_FORMAT_R8G8B8A8_SINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R32G32B32A32_SINT }; break; }
					}
					break;
				}
			}
			break;
		}


		case Format::DEPTH: {
			switch (format.precision) {
				case Format::HIGH_PRECISION: { candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT }; break; }
				case Format::MEDIUM_PRECISION: 
				case Format::LOW_PRECISION: { candidates = { VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT }; break; }
			}
			break;
		}
	}

    bool(*supported)(VkFormat, VkFormatFeatureFlags);
    switch (mode) {
    	case VK_IMAGE_TILING_LINEAR: {
			supported = [](VkFormat candidate, VkFormatFeatureFlags features)->bool { 
				VkFormatProperties properties;
				vkGetPhysicalDeviceFormatProperties(engine.gpu, candidate, &properties);
				return properties.linearTilingFeatures & features;
			};
			break;
		}
    	case VK_IMAGE_TILING_OPTIMAL: {
			supported = [](VkFormat candidate, VkFormatFeatureFlags features)->bool { 
				VkFormatProperties properties;
				vkGetPhysicalDeviceFormatProperties(engine.gpu, candidate, &properties);
				return properties.optimalTilingFeatures & features;
			};
			break;
		}
		default: {
			supported = [](VkFormat candidate, VkFormatFeatureFlags features)->bool { 
				VkFormatProperties properties;
				vkGetPhysicalDeviceFormatProperties(engine.gpu, candidate, &properties);
				return properties.bufferFeatures & features;
			};
			break;
		}
    }
    
	// validation loop
    for (VkFormat candidate : candidates) {
        if (supported(candidate, features))
            return candidate;
    }

	return VK_FORMAT_UNDEFINED;
}

Arawn::Image::Image(VkFormat format, VkImageUsageFlags usage, uint32_t width, uint32_t height, uint32_t mipmaps, VkSampleCountFlagBits sample_count) 
{
	VkImageCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent  = { width, height, 1 },
		.mipLevels = mipmaps,
		.arrayLayers = 1,
		.samples = sample_count,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VmaAllocationCreateInfo alloc {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
	};

	vmaCreateImage(engine.allocator, &info, &alloc, &image, &memory, nullptr);

	VkImageViewCreateInfo viewInfo {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, 
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = info.format,
		.subresourceRange = { (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, 0, mipmaps, 0, 1 }
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
*/