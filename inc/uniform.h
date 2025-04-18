#pragma once
#include "vulkan.h"
#include "buffer.h"
#include "texture.h"
#include <variant>

struct UniformBuffer { Buffer* ptr; };
struct StorageBuffer { Buffer* ptr; };
struct UniformTexture { Texture* ptr; };
struct InputAttachment { Texture* ptr; };
struct DepthAttachment { Texture* ptr; };
using Uniform = std::variant<UniformBuffer, StorageBuffer, UniformTexture, InputAttachment, DepthAttachment>;


class UniformSet {
public:
    UniformSet() : descriptor_set(nullptr) { }
    UniformSet(VK_TYPE(VkDescriptorSetLayout) layout, std::span<Uniform> bindings);
    ~UniformSet();
    UniformSet(UniformSet&&);
    UniformSet& operator=(UniformSet&&);
    UniformSet(const UniformSet&) = delete;
    UniformSet& operator=(const UniformSet&) = delete;

    void bind();

    VK_TYPE(VkDescriptorSet) descriptor_set;
};