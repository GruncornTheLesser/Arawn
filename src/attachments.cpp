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
        VkImageCreateInfo info{
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, 0, 
            VK_IMAGE_TYPE_2D, format, 
            { swapchain.extent.x, swapchain.extent.y, 1, }, 1, 1, 
            sample_count, VK_IMAGE_TILING_OPTIMAL, usage, 
            queue_families.size() < 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT, 
            static_cast<uint32_t>(queue_families.size()), queue_families.data()
        };

        for (uint32_t i = 0; i < settings.frame_count; ++i) {
            VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &image[i]));
        }

        if (settings.frame_count != MAX_FRAMES_IN_FLIGHT) {
            image[settings.frame_count] = nullptr;
        }
    }

    { // memory
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(engine.device, image[0], &requirements);

        VkMemoryAllocateInfo info{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, 
            requirements.size, engine.memory_type_index(requirements, memory_property) 
        };
        
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

    image = other.image;
    memory = other.memory;
    view = other.view;

    other.image[0] = nullptr;
}

TextureAttachment& TextureAttachment::operator=(TextureAttachment&& other) {
    if (this == &other) return *this;
    
    if (image[0] != nullptr) { 
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT && image[i]; ++i) {
            vkDestroyImage(engine.device, image[i], nullptr);
            vkDestroyImageView(engine.device, view[i], nullptr);
            vkFreeMemory(engine.device, memory[i], nullptr);
        }
    }

    image = other.image;
    memory = other.memory;
    view = other.view;

    other.image[0] = nullptr;

    return *this;
}

BufferAttachment::BufferAttachment(
    void* data, uint32_t size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memory_property, 
    std::span<uint32_t> queue_families)
{
    { // create buffer
        VkBufferCreateInfo info{ 
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0, 
            size, usage, 
            queue_families.size() < 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT, 
            static_cast<uint32_t>(queue_families.size()), queue_families.data()
        };

        for (uint32_t i = 0; i < settings.frame_count; ++i) {
            VK_ASSERT(vkCreateBuffer(engine.device, &info, nullptr, &buffer[i]));
        }

        if (settings.frame_count != MAX_FRAMES_IN_FLIGHT) {
            buffer[settings.frame_count] = nullptr;
        }
    }

    { // allocate memory
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(engine.device, buffer[0], &requirements);

        VkMemoryAllocateInfo info{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, 
            requirements.size, engine.memory_type_index(requirements, memory_property) 
        };
        
        for (uint32_t i = 0; i < settings.frame_count; ++i) {
            VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &memory[i]));
            VK_ASSERT(vkBindBufferMemory(engine.device, buffer[i], memory[i], 0));
        }
    }
}

BufferAttachment::~BufferAttachment() {
    if (buffer[0] == nullptr) return;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT && buffer[i]; ++i) {
        vkDestroyBuffer(engine.device, buffer[i], nullptr);
        vkFreeMemory(engine.device, memory[i], nullptr);
    }
}

BufferAttachment::BufferAttachment(BufferAttachment&& other) {
    if (this == &other) return;

    buffer = other.buffer;
    memory = other.memory;

    other.buffer[0] = nullptr;
}

BufferAttachment& BufferAttachment::operator=(BufferAttachment&& other) {
    if (this == &other) return *this;
    
    if (buffer[0] != nullptr) { 
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT && buffer[i]; ++i) {
            vkDestroyBuffer(engine.device, buffer[i], nullptr);
            vkFreeMemory(engine.device, memory[i], nullptr);
        }
    }

    buffer = other.buffer;
    memory = other.memory;

    other.buffer[0] = nullptr;

    return *this;
}
