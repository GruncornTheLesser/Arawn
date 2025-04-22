#version 450
#define MAX_LIGHTS_PER_CLUSTER 255

const float PI      = 3.14;
const float EPSILON = 0.01;

struct Light {
    vec3 position;
    float radius;
    vec3 colour;
    float intensity;
};

struct Frustum {
	vec4 planes[4];
};

struct Cluster {
	uint light_count;
	uint indices[MAX_LIGHTS_PER_CLUSTER];
};

layout(std140, set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
    mat4 inv_proj;
    uvec2 screen_size;
	float near;
	float far;
    vec3 eye;
};

layout(set = 1, input_attachment_index = 0, binding = 0) uniform subpassInputMS albedo_attachment;
layout(set = 1, input_attachment_index = 0, binding = 1) uniform subpassInputMS normal_attachment;
layout(set = 1, input_attachment_index = 0, binding = 2) uniform subpassInputMS position_attachment;

layout(std430, set=2, binding=0) readonly buffer LightArray { uvec3 cluster_count; uint light_count; Light lights[]; };
layout(std430, set=2, binding=1) readonly buffer FrustumArray { Frustum frustums[]; };
layout(std430, set=2, binding=2) readonly buffer ClusterArray { Cluster clusters[]; };
layout(set=2, binding=3) uniform sampler2DMS depth_sampler;

layout(location = 0) out vec4 out_colour;

vec3 F_Schlick(float HdotV, vec3 F0);
float D_GGX(float NdotH, float r);
float G_SchlickGGX(float NdotV, float roughness);
float G_Smith(float NdotV, float NdotL, float roughness);
float attenuate(vec3 light_position, vec3 frag_position, float radius, float intensity);
float linearize_depth_normalized(float depth);

void main() {
    vec4 in_albedo = subpassLoad(albedo_attachment, gl_SampleID);
    vec3 albedo = in_albedo.rgb;

    vec4 in_normal = subpassLoad(normal_attachment, gl_SampleID);
    vec3 normal = in_normal.rgb;
    float metallic = in_normal.a;

    vec4 in_position = subpassLoad(position_attachment, gl_SampleID);
    vec3 frag_position = in_position.rgb;
    float roughness = in_position.a;

    vec3 N = normalize(normal);
    vec3 V = normalize(frag_position - eye);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // ambient component
    out_colour = vec4(0.01 * albedo, 1.0);
    uvec3 clusterID = uvec3(
        vec2(gl_FragCoord.xy * cluster_count.xy) / screen_size.xy, 
        cluster_count.z * linearize_depth_normalized(texelFetch(depth_sampler, ivec2(gl_FragCoord.xy), gl_SampleID).r)
    );

    uint cluster_index = clusterID.x + 
                         clusterID.y * cluster_count.x + 
                         clusterID.z * cluster_count.x * cluster_count.y;
    for (uint i = 0; i < clusters[cluster_index].light_count; ++i) {
        
        Light light = lights[clusters[cluster_index].indices[i]];
        vec3 L = normalize(light.position - frag_position);
        vec3 H = normalize(V + L);

        float NdotH = max(dot(N, H), EPSILON);
        float NdotV = max(dot(N, V), EPSILON);
        float NdotL = max(dot(N, L), EPSILON);
        float HdotV = max(dot(H, V), EPSILON);
        
        float A = attenuate(light.position, frag_position, light.radius, light.intensity); // attenuation
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
    vec3 d = frag_position - light_position;                // difference
    float x2 = dot(d, d) / max(radius * radius, EPSILON);   // normalized dist squared 
    float f = offset * (x2 - 1);                            // offset
    return clamp(f / (f - x2), 0, 1);
}

float linearize_depth_normalized(float depth) {
    return near * depth / (far - depth * (far - near));
}

