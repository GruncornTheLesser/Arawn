#version 450

struct PointLight {
    vec3 position;
    float radius;
    vec3 intensity;
};

struct Cluster {
    vec3 a;
    vec3 b;
    uint point_light_count;
    uint point_light_indices[256];
    
    //uint probes_light_count;
    //uint probes_light_indices[256];
};

layout(push_constant) uniform PushConstant {
    ivec2 viewport_size;
    ivec3 cluster_count;
    int debug_view;
} push_constant;

layout(std140, set = 1, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
    vec3 eye;
};

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

layout(std430, set = 3, binding = 0) buffer readonly ClusterArray {
    Cluster clusters[];
};

layout(location = 0) in vec3 frag_position; // world position
layout(location = 1) in vec2 frag_texcoord;
layout(location = 2) in vec3 frag_normal;

layout(location = 0) out vec4 out_colour;

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

    
    vec3 normal;
    //if ((material.flags & normal_texture_flag) == normal_texture_flag) {
    //    normal = texture(normal_map, frag_texcoord).rgb;
    //}

    out_colour = vec4(albedo, 1.0);

}