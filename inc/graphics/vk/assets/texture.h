#pragma once
#include "vulkan.h"
#include <filesystem>
#include <span>

class Texture {
    friend class UniformSet;
    friend class DepthPass;
    friend class DeferredPass;
    friend class ForwardPass;
public:
    Texture() : image(nullptr) { }
    Texture(
        void* data, uint32_t width, uint32_t height, uint32_t mip_levels,
        VK_ENUM(VkFormat) format,
        VK_ENUM(VkImageUsageFlags) usage, 
        VK_ENUM(VkImageAspectFlagBits) aspect,
        VK_ENUM(VkSampleCountFlagBits) sample_count,
        VK_ENUM(VkMemoryPropertyFlags) memory_properties, 
        std::span<uint32_t> queue_families
    );
    Texture(std::filesystem::path fp, 
        VK_ENUM(VkImageUsageFlags) usage,
        VK_ENUM(VkImageAspectFlagBits) aspect, 
        VK_ENUM(VkSampleCountFlagBits) sample_count,
        VK_ENUM(VkMemoryPropertyFlags) memory_properties, 
        std::span<uint32_t> queue_families
    );
    Texture(std::filesystem::path fp);
    
    ~Texture();
    Texture(Texture&& other);
    Texture& operator=(Texture&& other);
    Texture(const Texture& other) = delete;
    Texture& operator=(const Texture& other) = delete;

// private:
    VK_TYPE(VkImage) image = nullptr;
    VK_TYPE(VkDeviceMemory) memory;
    VK_TYPE(VkImageView) view;
};