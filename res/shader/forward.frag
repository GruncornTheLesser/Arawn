#version 450

struct Light {
    vec3 position;
    float radius;
    vec3 colour;
    float intensity;
};


layout (std140, set = 0, binding = 0) uniform Camera {
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

layout(std430, set = 3, binding = 0) readonly buffer LightArray {
    uint light_count;
    int padding[3];
    Light lights[4096];
};

layout(location = 0) in vec3 frag_position; // world position
layout(location = 1) in vec2 frag_texcoord;
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 out_colour;

const uint albedo_texture_flag = 0x00000001;
const uint metallic_texture_flag = 0x00000002;
const uint roughness_texture_flag = 0x00000004;
const uint normal_texture_flag = 0x00000008;

const float PI      = 3.14;
const float EPSILON = 0.01;

vec3 F_Schlick(float HdotV, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

float D_GGX(float NdotH, float r) {
    float a = r * r;
    float a2 = a * a;
    float d = (NdotH * (a2 - 1.0) + 1.0);
    return a2 / max(PI * d * d, EPSILON);
}

float G_SchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1;
    float k = r * r / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, EPSILON);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
    return G_SchlickGGX(NdotL, roughness) * G_SchlickGGX(NdotV, roughness);
}

float attenuate(vec3 light_position, vec3 frag_position, float radius, float intensity) {
    const float offset = 0.1;
    vec3 d = light_position - frag_position;    // difference
    float x2 = dot(d, d) / (radius * radius);   // normalized dist squared 
    float f = offset * (x2 - 1); // offset
    return f / (f - x2);
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
        normal = texture(normal_map, frag_texcoord).rgb * 2.0 - 1.0;
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

    vec3 N = normalize(normal);
    vec3 V = normalize(frag_position - eye);

    vec3 F0 = vec3(0.01); // assumes IOR of ~1.5
    F0 = mix(F0, albedo, metallic);
    
    // ambient component
    out_colour = vec4(vec3(0.1) * albedo, 1.0);

    // for each light
    for (uint i = 0; i < light_count; ++i) {
        Light light = lights[i];
        
        vec3 L = normalize(light.position - frag_position);
        vec3 H = normalize(V + L);

        float NdotH = max(dot(N, H), EPSILON);
        float NdotV = max(dot(N, V), EPSILON);
        float NdotL = max(dot(N, L), 0.0);
        float HdotV = max(dot(H, V), 0.0);
        
        float A = attenuate(light.position, frag_position, light.radius, light.intensity); // attenuation
        float D = D_GGX(NdotH, roughness);
        float G = G_Smith(NdotV, NdotL, roughness);
        vec3  F = F_Schlick(HdotV, F0);

        vec3 diffuse = albedo / PI * (vec3(1.0) - F) * (1.0 - metallic);
        vec3 specular = D * G * F / max(4.0 * NdotV * NdotL, EPSILON);
        out_colour.rgb += (diffuse + specular) * light.colour * A * NdotL;
    }
}

