#define ARAWN_INCLUDE_VULKAN
#include <attachment/depth.h>
#include <core/settings.h>
#include <core/frame.h>
#include <core/device.h>
#include <algorithm>
#include <vector>

using namespace arawn::attachment;

void Depth::create(const core::Device &device, const core::Frame &frame, const core::Settings &settings) {
    std::vector<uint32_t> families = { device.queue.graphics.family, device.queue.compute.family };
    std::ranges::sort(families);
    families.erase(std::ranges::unique(families).begin(), families.end());

    format = device.selectFormat(
        std::to_array({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, }),
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
    );

    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { settings.resolution.x, settings.resolution.y, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT, // TODO:
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
        .pQueueFamilyIndices = families.data(),
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    
    VmaAllocationCreateInfo allocInfo {
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = { 
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }  
    };

    VmaAllocationInfo info{};
    for (uint32_t i = 0; i < frame.count; ++i) {
        VK_ASSERT(vmaCreateImage(device.allocator, &imageInfo, &allocInfo, &image[i], &memory[i], &info));
        
        viewInfo.image = image[i];
        VK_ASSERT(vkCreateImageView(device, &viewInfo, nullptr, &view[i]));
    }
}

void Depth::destroy(const core::Device &device) {
    for (uint32_t i = 0; i < ARAWN_MAX_IMAGES && image[i] != nullptr; ++i) {
        vkDestroyImageView(device, view[i], nullptr);
        vmaDestroyImage(device.allocator, image[i], memory[i]);
    }
}