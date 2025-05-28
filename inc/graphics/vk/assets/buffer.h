#pragma once
#include "vulkan.h"
#include <span>

class Buffer {
    friend class UniformSet;
    friend class Texture;
public:
    Buffer() : buffer(nullptr) { }
    Buffer(const void* data, uint32_t size, 
        VK_ENUM(VkBufferUsageFlags) usage,
        VK_ENUM(VkMemoryPropertyFlags) memory_properties, 
        std::span<uint32_t> queue_families
    );
    ~Buffer();
    Buffer(Buffer&& other);
    Buffer& operator=(Buffer&& other);
    Buffer(const Buffer& other) = delete;
    Buffer& operator=(const Buffer& other) = delete;

    void set_value(const void* data, uint32_t size, uint32_t offset = 0);

    VK_TYPE(VkBuffer) buffer = nullptr;
    VK_TYPE(VkDeviceMemory) memory;
};