#version 450

layout (std140, set=0, binding=0) uniform Camera {
    mat4 proj;
    mat4 view;
    mat4 inv_proj;
    uvec2 screen_size;
	float near;
	float far;
    vec3 eye;
};

layout (set=1, binding=0) uniform Transform {
    mat4 model;
};

layout(location = 0) in vec3 in_position;

void main() {
    mat4 mvp = proj * view * model;
    gl_Position = mvp * vec4(in_position, 1.0);

}