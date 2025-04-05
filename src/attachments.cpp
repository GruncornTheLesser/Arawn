#define ARAWN_IMPLEMENTATION
#include "attachments.h"
#include "engine.h"
#include "settings.h"
#include "swapchain.h"
TextureAttachment::TextureAttachment(
    VkImageUsageFlags usage, 
    VkFormat format, 
    VkImageAspectFlagBits aspect, 
    VkSampleCountFlagBits sample_count,
    VkMemoryPropertyFlags memory_property, 
    std::span<uint32_t> queue_families
) {
    { // image
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.extent = { swapchain.extent.x, swapchain.extent.y, 1 };
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.format = format;
        info.samples = sample_count;
        info.usage = usage;
        info.sharingMode = queue_families.size() < 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
        info.pQueueFamilyIndices = queue_families.data();
        info.queueFamilyIndexCount = queue_families.size();

        for (uint32_t i = 0; i < settings.frame_count; ++i)
            VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &image[i]));

        if (settings.frame_count != MAX_FRAMES_IN_FLIGHT) 
            image[settings.frame_count] = nullptr;
    }

    { // memory
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(engine.device, image[0], &requirements);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.pNext = nullptr;
        info.memoryTypeIndex = engine.memory_type_index(requirements, memory_property);
        info.allocationSize = requirements.size;
        
        for (uint32_t i = 0; i < settings.frame_count; ++i) {
            VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &memory[i]));
            VK_ASSERT(vkBindImageMemory(engine.device, image[i], memory[i], 0));
        }
    }

    { // view
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        info.format = format; 
        info.subresourceRange.aspectMask = aspect;

        for (uint32_t i = 0; i < settings.frame_count; ++i) {
            info.image = image[i];
            VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &view[i]));
        }
    }
}

TextureAttachment::~TextureAttachment() {
    if (image[0] == nullptr) return;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT && image[i]; ++i) {
        vkDestroyImage(engine.device, image[i], nullptr);
        vkDestroyImageView(engine.device, view[i], nullptr);
        vkFreeMemory(engine.device, memory[i], nullptr);
    }
}

TextureAttachment::TextureAttachment(TextureAttachment&& other) {
    if (this == &other) return;

    image = std::move(other.image);
    memory = std::move(other.memory);
    view = std::move(other.view);

    other.image[0] = nullptr;
}

TextureAttachment& TextureAttachment::operator=(TextureAttachment&& other) {
    if (this == &other) return *this;
    
    std::swap(image, other.image);
    std::swap(memory, other.memory);
    std::swap(view, other.view);

    return *this;
}
