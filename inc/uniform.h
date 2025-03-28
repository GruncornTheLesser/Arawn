#define ARAWN_IMPLEMENTATION
#pragma once
#include "vulkan.h"
#include <filesystem>
#include <span>

struct UniformSetBuilder {
    VK_TYPE(VkDescriptorSet) set;
    std::vector<VK_TYPE(VkWriteDescriptorSet)> desc_writes;
    std::vector<VK_TYPE(VkDescriptorBufferInfo)> buffer_info;
    std::vector<VK_TYPE(VkDescriptorImageInfo)> image_info;
};

struct Uniform { 
    virtual void write_set(UniformSetBuilder& set_builder, uint32_t binding_location) = 0;
};

template<typename T>
class UniformBuffer;

template<>
class UniformBuffer<void> : public Uniform {
public:
    UniformBuffer() : buffer(nullptr) { }
    UniformBuffer(const void* data, uint32_t size);
    ~UniformBuffer();
    UniformBuffer(UniformBuffer&& other);
    UniformBuffer& operator=(UniformBuffer&& other);
    UniformBuffer(const UniformBuffer& other) = delete;
    UniformBuffer& operator=(const UniformBuffer& other) = delete;

    void set(const void* data, uint32_t size);
    void* get() { return mapped_data; }
    const void* get() const { return mapped_data; }

    void write_set(UniformSetBuilder& set_builder, uint32_t binding_location, uint32_t size);

    VK_TYPE(VkBuffer) buffer = nullptr;
    VK_TYPE(VkDeviceMemory) memory;
    void* mapped_data;
};

template<typename T>
class UniformBuffer : public UniformBuffer<void> {
public:
    UniformBuffer() : UniformBuffer<void>() { }
    UniformBuffer(const T* data) : UniformBuffer<void>(data, sizeof(T)) { }
    UniformBuffer(const T& data) : UniformBuffer<void>(&data, sizeof(T)) { }
    UniformBuffer(UniformBuffer&& other) = default;
    UniformBuffer& operator=(UniformBuffer&& other) = default;
    UniformBuffer(const UniformBuffer& other) = delete;
    UniformBuffer& operator=(const UniformBuffer& other) = delete;

    void set(T* data) { UniformBuffer<void>::set(data, sizeof(T)); }
    T* get() { return reinterpret_cast<T*>(UniformBuffer<void>::get()); }
    const T* get() const { return reinterpret_cast<const T*>(UniformBuffer<void>::get()); }

    void write_set(UniformSetBuilder& set_builder, uint32_t binding_location) 
    {
        UniformBuffer<void>::write_set(set_builder, binding_location, sizeof(T));    
    }
};

class UniformTexture : public Uniform {
public:
    UniformTexture() : image(nullptr) { }
    UniformTexture(std::filesystem::path fp);
    ~UniformTexture();
    UniformTexture(UniformTexture&& other);
    UniformTexture& operator=(UniformTexture&& other);
    UniformTexture(const UniformTexture& other) = delete;
    UniformTexture& operator=(const UniformTexture& other) = delete;
    
    void write_set(UniformSetBuilder& set_builder, uint32_t binding_location);

private:
    VK_TYPE(VkImage) image = nullptr;
    VK_TYPE(VkDeviceMemory) memory;
    VK_TYPE(VkImageView) view;
};

class UniformSet {
public:
    UniformSet() { }
    UniformSet(VK_TYPE(VkDescriptorSetLayout) layout, std::span<Uniform*> binding);
    ~UniformSet();
    UniformSet(UniformSet&&);
    UniformSet& operator=(UniformSet&&);
    UniformSet(const UniformSet&) = delete;
    UniformSet& operator=(const UniformSet&) = delete;

    void bind(VK_TYPE(VkCommandBuffer) cmd_buffer, VK_TYPE(VkPipelineLayout) layout, uint32_t set_index, VK_ENUM(VkPipelineBindPoint) bind_point);

    VK_TYPE(VkDescriptorSet) descriptor_set = nullptr;
};
