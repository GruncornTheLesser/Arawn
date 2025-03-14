#version 450

layout (set = 0, binding = 0) uniform ModelMatrix {
    mat4 model; // transpose(inverse());
};

layout (std140, set = 1, binding = 0) uniform Camera {
    mat4 projview;
    vec3 position;
} camera;

layout(location = 0) in vec3 in_position;

void main() {
    mat4 mvp = camera.projview * model;
    gl_Position = mvp * vec4(in_position, 1.0);

}