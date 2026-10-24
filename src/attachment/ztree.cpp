#define ARAWN_INCLUDE_VULKAN
#include <attachment/ztree.h>
#include <core/settings.h>
#include <core/frame.h>
#include <core/device.h>
#include <algorithm>
#include <vector>

using namespace arawn::attachment;


void ZTree::create(const core::Device &device, const core::Frame &frame, const core::Settings &settings) {
    std::vector<uint32_t> families = { device.queue.graphics.family, device.queue.compute.family };
    std::ranges::sort(families);
    families.erase(std::ranges::unique(families).begin(), families.end());

    format = device.selectFormat(
        std::to_array({ VK_FORMAT_R16_SFLOAT, VK_FORMAT_R32_SFLOAT }),
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT
    );

    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { settings.resolution.x, settings.resolution.y, 1 },
        .mipLevels = std::min<uint32_t>(ARAWN_MAX_DEPTH_CHAIN, std::bit_width(std::min(settings.resolution.x, settings.resolution.y))),
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT,
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
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
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
        for (uint32_t j = 0; j < imageInfo.mipLevels; ++j) {
            viewInfo.subresourceRange.baseMipLevel = j;
            VK_ASSERT(vkCreateImageView(device, &viewInfo, nullptr, &mips[i][j]));
        }
    }
    if (imageInfo.mipLevels != ARAWN_MAX_DEPTH_CHAIN) {
        for (uint32_t i = 0; i < frame.count; ++i) {
            mips[i][imageInfo.mipLevels] = nullptr;
        }
    }
    if (frame.count != ARAWN_MAX_FRAMES) {
        image[frame.count] = nullptr;
    }
}

void ZTree::destroy(const core::Device &device) {
    for (uint32_t i = 0; i < ARAWN_MAX_IMAGES && image[i] != nullptr; ++i) {
        for (uint32_t j = 0; j < ARAWN_MAX_DEPTH_CHAIN && mips[i][j] != nullptr; ++j) {
            vkDestroyImageView(device, mips[i][j], nullptr);
        }
        vmaDestroyImage(device.allocator, image[i], memory[i]);
    }
}