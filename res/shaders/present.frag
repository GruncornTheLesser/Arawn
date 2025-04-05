#version 450

layout(set = 1, input_attachment_index = 0, binding = 0) uniform subpassInput albedo_attachment;
layout(set = 1, input_attachment_index = 1, binding = 0) uniform subpassInput normal_attachment;
layout(set = 1, input_attachment_index = 2, binding = 0) uniform subpassInput specular_attachment;

layout(location = 0) out vec4 out_colour;

void main() {
    vec4 in_albedo =   subpassLoad(albedo_attachment);
    vec4 in_normal =   subpassLoad(normal_attachment);
    vec4 in_specular = subpassLoad(specular_attachment);

    out_colour = vec4(in_albedo.xyz, 1);
}