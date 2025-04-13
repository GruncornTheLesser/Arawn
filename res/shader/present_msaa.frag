#version 450


layout(std140, set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
    vec3 eye;
};

layout(set = 1, input_attachment_index = 0, binding = 0) uniform subpassInputMS albedo_attachment;
layout(set = 1, input_attachment_index = 0, binding = 1) uniform subpassInputMS normal_attachment;
layout(set = 1, input_attachment_index = 0, binding = 2) uniform subpassInputMS position_attachment;

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
    vec4 in_albedo =   subpassLoad(albedo_attachment, gl_SampleID);
    vec4 in_normal =   subpassLoad(normal_attachment, gl_SampleID);
    vec4 in_position = subpassLoad(position_attachment, gl_SampleID);

    out_colour = vec4(in_albedo.rgb, 1);
}