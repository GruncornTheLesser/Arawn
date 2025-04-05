#pragma once
#include "vulkan.h"
#include <span>
#include <array>

struct TextureAttachment {
    TextureAttachment() { image[0] = nullptr; }
    TextureAttachment(
        VK_ENUM(VkImageUsageFlags) usage, 
        VK_ENUM(VkFormat) format, 
        VK_ENUM(VkImageAspectFlagBits) aspect, 
        VK_ENUM(VkSampleCountFlagBits) sample_count, 
        std::span<uint32_t> queue_families = { }
    );
    ~TextureAttachment();
    TextureAttachment(TextureAttachment&& other);
    TextureAttachment& operator=(TextureAttachment&& other);
    TextureAttachment(const TextureAttachment& other) = delete;
    TextureAttachment& operator=(const TextureAttachment& other) = delete;


    std::array<VK_TYPE(VkImage), MAX_FRAMES_IN_FLIGHT> image;
    std::array<VK_TYPE(VkImageView), MAX_FRAMES_IN_FLIGHT> view;
    std::array<VK_TYPE(VkDeviceMemory), MAX_FRAMES_IN_FLIGHT> memory;
};

struct BufferAttachment {
    BufferAttachment() { buffer[0] = nullptr; }
    ~BufferAttachment();
    BufferAttachment(BufferAttachment&& other);
    BufferAttachment& operator=(BufferAttachment&& other);
    BufferAttachment(const BufferAttachment& other) = delete;
    BufferAttachment& operator=(const BufferAttachment& other) = delete;


    std::array<VK_TYPE(VkBuffer), MAX_FRAMES_IN_FLIGHT> buffer;
    std::array<VK_TYPE(VkDeviceMemory), MAX_FRAMES_IN_FLIGHT> memory;
};
