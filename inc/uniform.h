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
    friend class UniformSet;
protected:
    virtual void set_binding(UniformSetBuilder& set_builder, uint32_t binding_location) = 0;
};

template<typename T>
class UniformBuffer;

template<>
class UniformBuffer<void> : public Uniform {
    friend class UniformSet;
public:
    UniformBuffer() : buffer(nullptr) { }
    UniformBuffer(const void* data, uint32_t size);
    ~UniformBuffer();
    UniformBuffer(UniformBuffer&& other);
    UniformBuffer& operator=(UniformBuffer&& other);
    UniformBuffer(const UniformBuffer& other) = delete;
    UniformBuffer& operator=(const UniformBuffer& other) = delete;

    void set_value(const void* data, uint32_t size);

protected:
    void set_binding(UniformSetBuilder& set_builder, uint32_t binding_location, uint32_t size);

    VK_TYPE(VkBuffer) buffer = nullptr;
    VK_TYPE(VkDeviceMemory) memory;
};

template<typename T>
class UniformBuffer : public UniformBuffer<void> {
    friend class UniformSet;
public:
    UniformBuffer() : UniformBuffer<void>() { }
    UniformBuffer(const T* data) : UniformBuffer<void>(data, sizeof(T)) { }
    UniformBuffer(const T& data) : UniformBuffer<void>(&data, sizeof(T)) { }
    UniformBuffer(UniformBuffer&& other) = default;
    UniformBuffer& operator=(UniformBuffer&& other) = default;
    UniformBuffer(const UniformBuffer& other) = delete;
    UniformBuffer& operator=(const UniformBuffer& other) = delete;

    void set_value(T& data) { UniformBuffer<void>::set_value(&data, sizeof(T)); }

protected:
    void set_binding(UniformSetBuilder& set_builder, uint32_t binding_location) {
        UniformBuffer<void>::set_binding(set_builder, binding_location, sizeof(T));
    }
};

class UniformTexture : public Uniform {
    friend class UniformSet;
public:
    UniformTexture() : image(nullptr) { }
    UniformTexture(std::filesystem::path fp);
    ~UniformTexture();
    UniformTexture(UniformTexture&& other);
    UniformTexture& operator=(UniformTexture&& other);
    UniformTexture(const UniformTexture& other) = delete;
    UniformTexture& operator=(const UniformTexture& other) = delete;
    
protected:
    void set_binding(UniformSetBuilder& set_builder, uint32_t binding_location);

    VK_TYPE(VkImage) image = nullptr;
    VK_TYPE(VkDeviceMemory) memory;
    VK_TYPE(VkImageView) view;
};

class UniformSet {
public:
    UniformSet() { }
    UniformSet(VK_TYPE(VkDescriptorSetLayout) layout);
    ~UniformSet();
    UniformSet(UniformSet&&);
    UniformSet& operator=(UniformSet&&);
    UniformSet(const UniformSet&) = delete;
    UniformSet& operator=(const UniformSet&) = delete;

    void set_bindings(std::span<Uniform*> binding);

    VK_TYPE(VkDescriptorSet) descriptor_set = nullptr;
};
