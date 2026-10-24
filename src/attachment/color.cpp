#define ARAWN_INCLUDE_VULKAN
#include <attachment/color.h>
#include <core/settings.h>
#include <core/frame.h>
#include <core/device.h>
#include <algorithm>
#include <vector>

using namespace arawn::attachment;

void Color::create(const core::Device &device, const core::Frame &frame, const core::Settings &settings) {
    std::vector<uint32_t> families = { device.queue.graphics.family, device.queue.present.family };
    std::ranges::sort(families);
    families.erase(std::ranges::unique(families).begin(), families.end());
    
    format = device.selectFormat(
        std::to_array({ VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM }), 
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
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
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
        .pQueueFamilyIndices = families.data(),
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocInfo {
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    
    VmaAllocationInfo info{};
    for (uint32_t i = 0; i < frame.count; ++i) {
        VK_ASSERT(vmaCreateImage(device.allocator, &imageInfo, &allocInfo, &image[i], &memory[i], &info));
    }
    if (frame.count != ARAWN_MAX_FRAMES) {
        image[frame.count] = nullptr;
    }
}

void Color::destroy(const core::Device &device) {
    for (uint32_t i = 0; i < ARAWN_MAX_IMAGES && image[i] != nullptr; ++i) {
        vmaDestroyImage(device.allocator, image[i], memory[i]);
    }
}