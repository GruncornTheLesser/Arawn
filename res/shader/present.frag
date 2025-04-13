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
    vec4 in_albedo =   subpassLoad(albedo_attachment);
    vec4 in_normal =   subpassLoad(normal_attachment);
    vec4 in_position = subpassLoad(position_attachment);

    vec3 albedo = in_albedo.rgb;
    vec3 normal = in_normal.rgb;
    float metallic = in_normal.a;
    vec3 frag_position = in_position.rgb;
    float roughness = in_position.a;
    
    
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