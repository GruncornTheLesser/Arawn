#define ARAWN_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "material.h"
#include "engine.h"

Material::Material() {
    tinyobj::material_t default_material;
    default_material.diffuse[0] = 0.0f;
    default_material.diffuse[1] = 0.0f;
    default_material.diffuse[2] = 0.0f;
    default_material.diffuse_texname = "res/image/default.png";
    default_material.normal_texname = "";
    default_material.roughness = 1.0f;
    default_material.roughness_texname = "";
    default_material.metallic = 0.0f;
    default_material.metallic_texname = "";
    default_material.normal_texname = "";

    *this = Material(&default_material, "");
}

Material::Material(const tinyobj::material_t* info, std::filesystem::path dir)
 : buffer(nullptr, sizeof(Data), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, { }),
   albedo(dir /= std::filesystem::path(info->diffuse_texname)), 
   metallic(dir /= std::filesystem::path(info->metallic_texname)), 
   roughness(dir /= std::filesystem::path(info->roughness_texname)), 
   normal(dir /= std::filesystem::path(info->normal_texname)),
   uniform(engine.material_layout, std::array<Uniform, 5>() = { 
        UniformBuffer{ &buffer }, 
        UniformTexture{ &albedo }, 
        UniformTexture{ &metallic }, 
        UniformTexture{ &roughness }, 
        UniformTexture{ &normal }
    })
{
    Data data;
    data.albedo = { info->diffuse[0], info->diffuse[1], info->diffuse[2] };
    data.metallic = info->metallic;
    data.roughness = info->roughness;
    data.flags = 0;

    if (!info->diffuse_texname.empty()) data.flags |= Material::ALBEDO_TEXTURE;
    if (!info->metallic_texname.empty()) data.flags |= Material::METALLIC_TEXTURE;
    if (!info->roughness_texname.empty()) data.flags |= Material::ROUGHNESS_TEXTURE;
    if (!info->normal_texname.empty()) data.flags |= Material::NORMAL_TEXTURE;

    buffer.set_value(&data, sizeof(Data));
}