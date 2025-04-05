#version 450

layout (std140, set = 2, binding = 0) uniform Material {
    vec3 albedo;
    float metallic;
    float roughness;
    uint flags;
} material;

layout(set = 2, binding = 1) uniform sampler2D albedo_map;
layout(set = 2, binding = 2) uniform sampler2D metallic_map;
layout(set = 2, binding = 3) uniform sampler2D roughness_map;
layout(set = 2, binding = 4) uniform sampler2D normal_map;

layout(location = 0) in vec3 frag_position; // world position
layout(location = 1) in vec2 frag_texcoord;
layout(location = 2) in vec3 frag_normal;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_specular;


const uint albedo_texture_flag = 0x00000001;
const uint metallic_texture_flag = 0x00000002;
const uint roughness_texture_flag = 0x00000004;
const uint normal_texture_flag = 0x00000008;

void main() {
    if ((material.flags & albedo_texture_flag) == albedo_texture_flag) {
        out_albedo = vec4(texture(albedo_map, frag_texcoord).rgb, 1);
    } else {
        out_albedo = vec4(material.albedo, 1);
    }

    vec3 normal = frag_normal;
    //if ((material.flags & normal_texture_flag) == normal_texture_flag) {
    //    normal = texture(normal_map, frag_texcoord).rgb;
    //}

    if ((material.flags & metallic_texture_flag) == metallic_texture_flag) {
        out_specular.r = texture(metallic_map, frag_texcoord).r;
    } else {
        out_specular.r = material.metallic;
    }

    if ((material.flags & roughness_texture_flag) == roughness_texture_flag) {
        out_specular.g = texture(roughness_map, frag_texcoord).r;
    } else {
        out_specular.g = material.roughness;
    }
}