#pragma once
#include "vulkan.h"

#include <filesystem>
struct Texture {
    Texture() { }
    Texture(std::filesystem::path fp, VK_ENUM(VkFormat) format);
    ~Texture();
    Texture(Texture&& other);
    Texture& operator=(Texture&& other);
    Texture(const Texture& other) = delete;
    Texture& operator=(const Texture& other) = delete;
    
    VK_TYPE(VkImage) image = nullptr;
    VK_TYPE(VkDeviceMemory) memory;
    VK_TYPE(VkImageView) view;
};
