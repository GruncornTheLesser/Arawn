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
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 out_albedo; // albedo + alpha
layout(location = 1) out vec4 out_normal; // normal + metallic
layout(location = 2) out vec4 out_position; // position + roughness


const uint albedo_texture_flag = 0x00000001;
const uint metallic_texture_flag = 0x00000002;
const uint roughness_texture_flag = 0x00000004;
const uint normal_texture_flag = 0x00000008;

void main() {
    vec3 albedo;
    if ((material.flags & albedo_texture_flag) == albedo_texture_flag) {
        albedo = texture(albedo_map, frag_texcoord).rgb;
    } else {
        albedo = material.albedo;
    }

    vec3 normal;
    if ((material.flags & normal_texture_flag) == normal_texture_flag) {
        vec3 normal = texture(normal_map, frag_texcoord).rgb * 2.0 - 1.0;
        normal = normalize(TBN * normal);
    } else {
        normal = TBN[2];
    }

    float metallic;
    if ((material.flags & metallic_texture_flag) == metallic_texture_flag) {
        metallic = texture(metallic_map, frag_texcoord).r;
    } else {
        metallic = material.metallic;
    }

    float roughness;
    if ((material.flags & roughness_texture_flag) == roughness_texture_flag) {
        roughness = texture(roughness_map, frag_texcoord).r;
    } else {
        roughness = material.roughness;
    }

    out_albedo.rgb = albedo.rgb;

    out_normal.rgb = normal.rgb;
    out_normal.a   = metallic;

    out_position.rgb = frag_position.rgb;
    out_position.a   = roughness;
}