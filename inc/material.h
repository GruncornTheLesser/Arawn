#pragma once
#include "vulkan.h"
#include "uniform.h"

#ifdef ARAWN_IMPLEMENTATION
#include <tiny_obj_loader.h>
#endif

struct Material {
    enum Flags { ALBEDO_TEXTURE = 1, METALLIC_TEXTURE = 2, ROUGHNESS_TEXTURE = 4, NORMAL_TEXTURE = 8 };
    struct Data {
        // note c++ alignas doesnt work the way align works for std140 standard glsl
        glm::vec3 albedo;   // offset 0, align 12
        float metallic;     // offset 12, align 4
        float roughness;    // offset 16, align 4
        uint32_t flags;     // offset 20, algin 4
    };
    
    Material(); // default material
    Material(VK_TYPE(const tinyobj::material_t*) material);

    UniformBuffer<Data> buffer;
    UniformTexture albedo_texture, metallic_texture, roughness_texture, normal_texture;
    UniformSet uniform;
};