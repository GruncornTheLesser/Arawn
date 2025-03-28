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
 : buffer(nullptr), // initialize with no data
   albedo_texture(info->diffuse_texname), 
   metallic_texture(info->metallic_texname), 
   roughness_texture(info->roughness_texname), 
   normal_texture(info->normal_texname),
   uniform(engine.material_layout, std::array<Uniform*, 5>() = { &buffer, &albedo_texture, &metallic_texture, &roughness_texture, &normal_texture })
{
    Data* mapped_data = buffer.get();

    mapped_data->albedo = { info->diffuse[0], info->diffuse[1], info->diffuse[2] };
    mapped_data->metallic = info->metallic;
    mapped_data->roughness = info->roughness;

    if (!info->diffuse_texname.empty()) {
        mapped_data->flags |= Material::ALBEDO_TEXTURE;
        albedo_texture = { info->diffuse_texname };
    }

    if (!info->metallic_texname.empty()) {
        mapped_data->flags |= Material::METALLIC_TEXTURE;
        metallic_texture = { info->metallic_texname };
    }

    if (!info->roughness_texname.empty()) {
        mapped_data->flags |= Material::ROUGHNESS_TEXTURE;
        roughness_texture = { info->roughness_texname };
    }

    if (!info->normal_texname.empty()) {
        mapped_data->flags |= Material::NORMAL_TEXTURE;
        normal_texture = { info->normal_texname };
    }
}