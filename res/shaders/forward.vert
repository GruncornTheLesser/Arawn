#version 450

layout (set = 0, binding = 0) uniform ModelMatrix {
    mat4 model; // transpose(inverse());
};

layout (std140, set = 1, binding = 0) buffer readonly Camera {
    mat4 projview;
    vec3 position;
} camera;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_colour;
layout(location = 2) in vec2 in_texcoord;
//layout(location = 3) in vec3 in_normal;

layout(location = 0) out vec3 frag_colour;
layout(location = 1) out vec2 frag_texcoord;
//layout(location = 2) out vec3 frag_normal;
layout(location = 3) out vec3 frag_position; // world position


void main() {
    mat4 mvp = camera.projview * model;
    gl_Position = mvp * vec4(in_position, 1.0);
    frag_colour = in_colour;
    frag_texcoord = in_texcoord;
    //frag_normal = normalize(vec3(model * vec4(in_normal, 0.0)));
    //frag_position = vec3(model * vec4(in_position, 1.0));

}