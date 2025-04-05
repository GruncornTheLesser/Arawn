#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput in_albedo;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput in_normal;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput in_specular;

layout(location = 0) out vec4 out_colour;

void main() {
    out_colour = vec4(1, 0, 0, 1);    
}