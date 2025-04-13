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
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 out_colour;

const uint albedo_texture_flag = 0x00000001;
const uint metallic_texture_flag = 0x00000002;
const uint roughness_texture_flag = 0x00000004;
const uint normal_texture_flag = 0x00000008;

const float PI = 3.141592653589793;
const float EPSILON = 0.00001;

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistGGX(float NdotH, float r) {
    float a = r * r;
    float a2 = a * a;
    float d = (NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * d * d);
}

float GeomSchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1;
    float k = r * r / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeomSmith(float NdotV, float NdotL, float roughness) {
    return GeomSchlickGGX(NdotL, roughness) * GeomSchlickGGX(NdotV, roughness);
}

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

    vec3 N = normal;
    vec3 V = normalize(eye - frag_position);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    vec3 radiance = vec3(0.0);
    // for each light
    {
        vec3 light_position = vec3(5, 5, 0);
        vec3 light_dir = normalize(light_position - frag_position);
        vec3 light_colour = vec3(10.0, 10.0, 10.0);
        
        vec3 L = normalize(light_position - frag_position);
        vec3 H = normalize(V + L);

        float dist = length(light_position - frag_position);

        float NdotH = max(dot(N, H), 0.0); 
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        float HdotV = max(dot(H, V), 0.0);

        float NDF = DistGGX(NdotH, roughness);
        float G = GeomSmith(NdotV, NdotL, roughness);
        vec3 F = FresnelSchlick(HdotV, F0);

        vec3 diffuse = (vec3(1.0) - F) * (1.0 - metallic);
        vec3 specular = NDF * G * F / (4.0 * NdotV * NdotL + EPSILON);
        radiance += (diffuse * albedo / PI + specular) * light_colour * NdotL / (dist * dist);
    }

    vec3 ambient = vec3(0.03) * albedo;
    vec3 colour = ambient + radiance;
	   
    out_colour = vec4(colour, 1.0);
}

