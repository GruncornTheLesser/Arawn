#define ARAWN_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include "material.h"
#include "engine.h"

Material::Material() {
    Material::Data data;
    data.flags |= Material::ALBEDO_TEXTURE;
    albedo_texture = { "res/images/default.png", VK_FORMAT_R8G8B8A8_UNORM };
}

Material::Material(const tinyobj::material_t* info) {
    Material::Data data;
    data.albedo = { info->diffuse[0], info->diffuse[1], info->diffuse[2] };
    data.metallic = info->metallic;
    data.roughness = info->roughness;

    if (!info->diffuse_texname.empty()) {
        data.flags |= Material::ALBEDO_TEXTURE;
        albedo_texture = { info->diffuse_texname, VK_FORMAT_R8G8B8A8_UNORM };
    }

    if (!info->metallic_texname.empty()) {
        data.flags |= Material::METALLIC_TEXTURE;
        metallic_texture = { info->metallic_texname, VK_FORMAT_R8_UNORM };
    }

    if (!info->roughness_texname.empty()) {
        data.flags |= Material::ROUGHNESS_TEXTURE;
        roughness_texture = { info->roughness_texname, VK_FORMAT_R8_UNORM };
    }

    if (!info->normal_texname.empty()) {
        data.flags |= Material::NORMAL_TEXTURE;
        normal_texture = { info->normal_texname, VK_FORMAT_R8G8B8_UNORM };
    }

    // create uniform buffer
}

Material::~Material() {

}

Material::Material(Material&& other) : 
    albedo_texture(std::move(other.albedo_texture)),
    metallic_texture(std::move(other.metallic_texture)),
    roughness_texture(std::move(other.roughness_texture)),
    normal_texture(std::move(other.normal_texture))
{ }
Material& Material::operator=(Material&& other) {
    albedo_texture = std::move(other.albedo_texture);
    metallic_texture = std::move(other.metallic_texture);
    roughness_texture = std::move(other.roughness_texture);
    normal_texture = std::move(other.normal_texture);
}