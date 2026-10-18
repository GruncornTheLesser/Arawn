#define ARAWN_INCLUDE_VULKAN
#include <resources/image.h>
#include <engine/device.h>
#include <vector>
#include <ranges>
#include <algorithm>



using namespace arawn::resources;

void Image::create(
    const engine::Device &device,
    uint32_t width, uint32_t height,
    uint32_t layers, uint32_t levels,
    VkFormat format, VkSampleCountFlagBits samples, 
    VkImageUsageFlags usage,
    VmaAllocationCreateFlags flags,
    std::initializer_list<uint32_t> queues
) {
    std::vector<uint32_t> families(queues);
    std::ranges::sort(families);
    families.erase(std::ranges::unique(families).begin(), families.end());

    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { width, height, 1 },
        .mipLevels = levels,
        .arrayLayers = layers,
        .samples = samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
        .pQueueFamilyIndices = families.data(),
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    
    VmaAllocationCreateInfo allocInfo {
        .flags = flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VmaAllocationInfo info{};
    VK_ASSERT(vmaCreateImage(device.allocator, &imageInfo, &allocInfo, &image, &memory, &info));
}

VkImageView Image::createView(const engine::Device& device, VkFormat format, VkImageAspectFlagBits aspect, uint32_t level, uint32_t count) const {
    VkImageView view;
    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = { 
            .aspectMask = aspect,
            .baseMipLevel = level,
            .levelCount = count,
            .baseArrayLayer = 0,
            .layerCount = 1
        }  
    };
    VK_ASSERT(vkCreateImageView(device.device, &viewInfo, nullptr, &view));
    return view;
}


void Image::destroy(const engine::Device &device) {
    vmaDestroyImage(device.allocator, image, memory);
}