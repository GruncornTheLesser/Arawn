#version 450


layout (std140, set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
    int test;
};
layout (set = 1, binding = 0) uniform Object {
    mat4 model;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 in_tangent;

layout(location = 0) out vec3 frag_position; // world position
layout(location = 1) out vec2 frag_texcoord;
layout(location = 2) out vec3 frag_normal;

mat4 _proj = mat4(2.4142, 0.0, 0.0, 0.0, 
                  0.0, 2.4142, 0.0, 0.0, 
                  0.0, 0.0, -1.0002, -0.20002, 
                  0.0, 0.0, -1.0, 0.0

);
mat4 _view = mat4(1.0, 0.0, 0.0, 0.0, 
                  0.0, 1.0, 0.0, 0.0, 
                  0.0, 0.0,-1.0,-5.0, 
                  0.0, 0.0, 0.0, 1.0

);
mat4 _model = mat4(1.0, 0.0, 0.0, 0.0, 
                   0.0, 1.0, 0.0, 0.0, 
                   0.0, 0.0, 1.0, 0.0, 
                   0.0, 0.0, 0.0, 1.0 
);

vec3 vertices[3] = {
    { 0.5, 0.0, 0.0 }, 
    { 1.0, 1.0, 0.0 }, 
    { 0.0, 1.0, 0.0 }
};

void main() {
    gl_Position = proj * view * model * vec4(in_position, 1.0);

    frag_position = vec3(model * vec4(in_position, 1.0));
    frag_texcoord = in_texcoord;
    frag_normal = normalize(vec3(model * vec4(in_normal, 0.0)));
}