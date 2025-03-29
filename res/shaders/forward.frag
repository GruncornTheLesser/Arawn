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
    mat4 projview;
    vec3 position;
} camera;

layout (std140, set = 2, binding = 0) uniform Material {
    vec3 albedo;
    float metallic;
    float roughness;
    uint flags;
};

layout(set = 2, binding = 1) uniform sampler albedo_map;
layout(set = 2, binding = 2) uniform sampler metallic_map;
layout(set = 2, binding = 3) uniform sampler roughness_map;
layout(set = 2, binding = 4) uniform sampler normal_map;

layout(std430, set = 3, binding = 0) buffer readonly ClusterArray {
    Cluster clusters[];
};

layout(location = 0) in vec3 frag_position; // world position
layout(location = 1) in vec2 frag_texcoord;
layout(location = 2) in vec3 frag_normal;

layout(location = 0) out vec4 out_colour;

void main() {
    out_colour = vec4(1.0, 0.0, 0.0, 1.0);

}