#pragma once
#include "vulkan.h"
#include "texture.h"
#include <filesystem>

struct Material {
    enum Flags { ALBEDO_TEXTURE = 1, METALLIC_TEXTURE = 2, ROUGHNESS_TEXTURE = 4, NORMAL_TEXTURE = 8 };
    struct Data {
        // note c++ alignas doesnt work the way alignas works for std140 standard glsl
        glm::vec3 albedo;   // offset 0, align 12
        float metallic;     // offset 12, align 4
        float roughness;    // offset 16, align 4
        uint32_t flags;     // offset 20, algin 4
    };
    
    Material(); // default material
    Material(VK_TYPE(const tinyobj::material_t*) material);
    ~Material();
    Material(Material&& other);
    Material& operator=(Material&& other);
    Material(const Material& other) = delete;
    Material& operator=(const Material& other) = delete;

    Texture albedo_texture, metallic_texture, roughness_texture, normal_texture;

    VK_TYPE(VkDescriptorSet) set;
    VK_TYPE(VkBuffer) buffer;
    VK_TYPE(VkDeviceMemory) memory;
};