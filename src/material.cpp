#define ARAWN_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "material.h"
#include "engine.h"

Material::Material() {
    tinyobj::material_t default_material;
    default_material.diffuse[0] = 0.0f;
    default_material.diffuse[1] = 0.0f;
    default_material.diffuse[2] = 0.0f;
    default_material.diffuse_texname = "res/images/default.png";
    default_material.normal_texname = "";
    default_material.roughness = 0.0f;
    default_material.roughness_texname = "";
    default_material.metallic = 0.0f;
    default_material.metallic_texname = "";
    default_material.normal_texname = "";

    *this = Material(&default_material);
}

Material::Material(const tinyobj::material_t* info)
 : buffer(nullptr, sizeof(Data)),
   albedo_texture(info->diffuse_texname), 
   metallic_texture(info->metallic_texname), 
   roughness_texture(info->roughness_texname), 
   normal_texture(info->normal_texname),
   set(engine.material_layout, std::array<std::variant<UniformBuffer*, UniformTexture*>, 5>() = { &buffer, &albedo_texture, &metallic_texture, &roughness_texture, &normal_texture })
{
    Data data;
    data.albedo = { info->diffuse[0], info->diffuse[1], info->diffuse[2] };
    data.metallic = info->metallic;
    data.roughness = info->roughness;

    if (!info->diffuse_texname.empty()) data.flags |= Material::ALBEDO_TEXTURE;
    if (!info->metallic_texname.empty()) data.flags |= Material::METALLIC_TEXTURE;
    if (!info->roughness_texname.empty()) data.flags |= Material::ROUGHNESS_TEXTURE;
    if (!info->normal_texname.empty()) data.flags |= Material::NORMAL_TEXTURE;

    buffer.set_value(&data);
}