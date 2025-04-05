#version 450

layout(set = 1, input_attachment_index = 0, binding = 0) uniform subpassInputMS albedo_attachment;
layout(set = 1, input_attachment_index = 1, binding = 0) uniform subpassInputMS normal_attachment;
layout(set = 1, input_attachment_index = 2, binding = 0) uniform subpassInputMS specular_attachment;

layout(location = 0) out vec4 out_colour;

void main() {
    vec4 in_albedo =   subpassLoad(albedo_attachment, gl_SampleID);
    vec4 in_normal =   subpassLoad(normal_attachment, gl_SampleID);
    vec4 in_specular = subpassLoad(specular_attachment, gl_SampleID);

    out_colour = vec4(in_albedo.xyz, 1);
}