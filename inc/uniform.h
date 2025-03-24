#pragma once
#include "vulkan.h"

template<typename T>
class UniformBuffer;

template<>
class UniformBuffer<void> {
public:
    UniformBuffer(uint32_t size);
    ~UniformBuffer();
    UniformBuffer(UniformBuffer&& other);
    UniformBuffer& operator=(UniformBuffer&& other);
    UniformBuffer(const UniformBuffer& other) = delete;
    UniformBuffer& operator=(const UniformBuffer& other) = delete;

    void set(void* data, uint32_t size, uint32_t offset = 0);
    void* get(uint32_t offset = 0) { return mapped_data; }
    const void* get(uint32_t offset = 0) const { return mapped_data; }

   VK_TYPE(VkBuffer) buffer;
   VK_TYPE(VkDeviceMemory) memory;
   void* mapped_data;
};

template<typename T>
class UniformBuffer : public UniformBuffer<void> {
public:
    UniformBuffer() : UniformBuffer<void>(sizeof(T)) { }
    UniformBuffer(UniformBuffer&& other) = default;
    UniformBuffer& operator=(UniformBuffer&& other) = default;
    UniformBuffer(const UniformBuffer& other) = delete;
    UniformBuffer& operator=(const UniformBuffer& other) = delete;

    void set(T* data, uint32_t offset = 0) { UniformBuffer<void>::set(data, sizeof(T), offset); }
    T* get(uint32_t offset = 0) { return reinterpret_cast<T*>(UniformBuffer<void>::get(offset)); }
    const T* get(uint32_t offset = 0) const { return reinterpret_cast<const T*>(UniformBuffer<void>::get(offset)); }
};
