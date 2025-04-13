#version 450

layout(std140, set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
    vec3 eye;
};

layout(set = 1, input_attachment_index = 0, binding = 0) uniform subpassInput albedo_attachment;
layout(set = 1, input_attachment_index = 0, binding = 1) uniform subpassInput normal_attachment;
layout(set = 1, input_attachment_index = 0, binding = 2) uniform subpassInput position_attachment;

layout(location = 0) out vec4 out_colour;

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

void main() {
    vec4 in_albedo = subpassLoad(albedo_attachment);
    vec3 albedo = in_albedo.rgb;

    vec4 in_normal = subpassLoad(normal_attachment);
    vec3 normal = in_normal.rgb;
    float metallic = in_normal.a;

    vec4 in_position = subpassLoad(position_attachment);
    vec3 frag_position = in_position.rgb;
    float roughness = in_position.a;


    vec3 N = normalize(normal);
    vec3 V = normalize(frag_position - eye);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // ambient component
    out_colour = vec4(vec3(0.03) * albedo, 1.0);

    // for each light
    {
        vec3 light_position = vec3(0, 5, 0);
        vec3 light_colour = vec3(100.0, 100.0, 100.0);
        
        vec3 L = normalize(light_position - frag_position);
        vec3 H = normalize(V + L);

        float dist = max(length(light_position - frag_position), EPSILON);

        float NdotH = max(dot(N, H), EPSILON); 
        float NdotV = max(dot(N, V), EPSILON);
        float NdotL = max(dot(N, L), 0.0);
        float HdotV = max(dot(H, V), 0.0);

        float A = NdotL / (dist * dist); // attenuation
        float D = D_GGX(NdotH, roughness);
        float G = G_Smith(NdotV, NdotL, roughness);
        vec3  F = F_Schlick(HdotV, F0);

        vec3 diffuse = albedo / PI * (vec3(1.0) - F) * (1.0 - metallic);
        vec3 specular = D * G * F / max(4.0 * NdotV * NdotL, EPSILON);
        out_colour.rgb += (vec3(F)) * light_colour * A;
    }
}