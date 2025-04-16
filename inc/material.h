#pragma once
#include "vulkan.h"
#include "uniform.h"

#ifdef ARAWN_IMPLEMENTATION
#include <tiny_obj_loader.h>
#endif

struct Material {
    enum Flags { ALBEDO_TEXTURE = 1, METALLIC_TEXTURE = 2, ROUGHNESS_TEXTURE = 4, NORMAL_TEXTURE = 8 };
    struct Data {
        glm::vec3 albedo;
        float metallic;
        float roughness;
        uint32_t flags;
    };
    
    Material(); // default material
    Material(VK_TYPE(const tinyobj::material_t*) material, std::filesystem::path dir);

    UniformBuffer buffer;
    UniformTexture albedo_texture, metallic_texture, roughness_texture, normal_texture;
    UniformSet set;
};