#version 450
#define TILE_SIZE 16
#define MAX_LIGHTS_PER_TILE 127

const float PI      = 3.14;
const float EPSILON = 0.01;

struct Light {
    vec3 position;
    float radius;
    vec3 colour;
    float curve;
};

struct Frustum {
	vec4 planes[4];
};

struct Cluster {
	uint light_count;
	uint indices[MAX_LIGHTS_PER_TILE];
};


layout (std140, set=0, binding=0) uniform Camera {
    mat4 proj;
    mat4 view;
    mat4 inv_proj;
    uvec2 screen_size;
	float near;
	float far;
    vec3 eye;
};

// layout (std140, set=1, binding=0) uniform Transform; 
layout (std140, set=2, binding=0) uniform Material {
    vec3 albedo;
    float metallic;
    float roughness;
    uint flags;
} material;
layout(set=2, binding=1) uniform sampler2D albedo_map;
layout(set=2, binding=2) uniform sampler2D metallic_map;
layout(set=2, binding=3) uniform sampler2D roughness_map;
layout(set=2, binding=4) uniform sampler2D normal_map;

layout(std430, set=3, binding=0) readonly buffer LightArray { uvec3 cluster_count; uint light_count; Light lights[]; };
layout(std430, set=3, binding=1) readonly buffer FrustumArray { Frustum frustums[]; };
layout(std430, set=3, binding=2) readonly buffer ClusterArray { Cluster clusters[]; };

layout(location = 0) in vec3 frag_position; // world position
layout(location = 1) in vec2 frag_texcoord;
layout(location = 2) in mat3 TBN;

layout(location = 0) out vec4 out_colour;

const uint albedo_texture_flag = 0x00000001;
const uint metallic_texture_flag = 0x00000002;
const uint roughness_texture_flag = 0x00000004;
const uint normal_texture_flag = 0x00000008;

vec3 F_Schlick(float HdotV, vec3 F0);
float D_GGX(float NdotH, float r);
float G_SchlickGGX(float NdotV, float roughness);
float G_Smith(float NdotV, float NdotL, float roughness);
float attenuate(vec3 light_position, vec3 frag_position, float radius, float curve);

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
    vec3 V = normalize(eye - frag_position);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // ambient component
    out_colour = vec4(0.01 * albedo, 1.0);
    
    uvec2 tilecoord = uvec2(gl_FragCoord) / ((screen_size.xy - 1) / cluster_count.xy + 1);
    uint cluster_index = tilecoord.x + tilecoord.y * cluster_count.x;
    for (uint i = 0; i < clusters[cluster_index].light_count; ++i) {
        
        Light light = lights[clusters[cluster_index].indices[i]];
        vec3 L = normalize(light.position - frag_position);
        vec3 H = normalize(V + L);

        float NdotH = max(dot(N, H), EPSILON);
        float NdotV = max(dot(N, V), EPSILON);
        float NdotL = max(dot(N, L), EPSILON);
        float HdotV = max(dot(H, V), EPSILON);
        
        float A = attenuate(light.position, frag_position, light.radius, light.curve); // attenuation
        float D = D_GGX(NdotH, roughness);
        float G = G_Smith(NdotV, NdotL, roughness);
        vec3  F = F_Schlick(HdotV, F0);

        vec3 diffuse = albedo / max(PI * (1.0 - metallic), EPSILON);
        vec3 specular = D * G * F / max(4.0 * NdotV * NdotL, EPSILON);
        out_colour.rgb += (diffuse + specular) * light.colour * A * NdotL;
    }   
}

vec3 F_Schlick(float HdotV, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

float D_GGX(float NdotH, float roughness) {
    float a = roughness * roughness;
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

float attenuate(vec3 light_position, vec3 frag_position, float radius, float curve) {
    vec3 d = light_position - frag_position;    // difference
    float d2 = dot(d, d);
    float r2 = radius * radius;
    return clamp(curve * max(r2 - d2, 0) / d2, 0, 1);
}