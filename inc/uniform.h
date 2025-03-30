#define ARAWN_IMPLEMENTATION
#pragma once
#include "vulkan.h"
#include <filesystem>
#include <variant>
#include <span>

class UniformBuffer {
    friend class UniformSet;
public:
    UniformBuffer() : buffer(nullptr) { }
    UniformBuffer(const void* data, uint32_t size);
    ~UniformBuffer();
    UniformBuffer(UniformBuffer&& other);
    UniformBuffer& operator=(UniformBuffer&& other);
    UniformBuffer(const UniformBuffer& other) = delete;
    UniformBuffer& operator=(const UniformBuffer& other) = delete;

    void set_value(const void* data);

private:
    VK_TYPE(VkBuffer) buffer = nullptr;
    VK_TYPE(VkDeviceMemory) memory;
    uint32_t size;
};

class UniformTexture {
    friend class UniformSet;
public:
    UniformTexture() : image(nullptr) { }
    UniformTexture(std::filesystem::path fp);
    ~UniformTexture();
    UniformTexture(UniformTexture&& other);
    UniformTexture& operator=(UniformTexture&& other);
    UniformTexture(const UniformTexture& other) = delete;
    UniformTexture& operator=(const UniformTexture& other) = delete;

private:
    VK_TYPE(VkImage) image = nullptr;
    VK_TYPE(VkDeviceMemory) memory;
    VK_TYPE(VkImageView) view;
};

class UniformSet {
public:
    UniformSet() { }
    UniformSet(VK_TYPE(VkDescriptorSetLayout) layout, std::span<std::variant<UniformBuffer*, UniformTexture*>> bindings);
    ~UniformSet();
    UniformSet(UniformSet&&);
    UniformSet& operator=(UniformSet&&);
    UniformSet(const UniformSet&) = delete;
    UniformSet& operator=(const UniformSet&) = delete;

    void bind();

    VK_TYPE(VkDescriptorSet) descriptor_set = nullptr;
};