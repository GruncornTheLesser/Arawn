#version 450


layout (std140, set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
    mat4 inv_proj;
    uvec2 screen_size;
	float near;
	float far;
    vec3 eye;
};
layout (set = 1, binding = 0) uniform Transform {
    mat4 model;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec3 in_tangent;
layout(location = 4) in vec3 in_bi_tangent;

layout(location = 0) out vec3 frag_position;
layout(location = 1) out vec2 frag_texcoord;
layout(location = 2) out mat3 frag_TBN;


void main() {
    gl_Position = proj * view * model * vec4(in_position, 1.0);

    frag_position = vec3(model * vec4(in_position, 1.0));
    frag_texcoord = in_texcoord;
    mat3 norm_mat = transpose(inverse(mat3(model)));
    frag_TBN = mat3(norm_mat * in_tangent, 
                    norm_mat * in_bi_tangent, 
                    norm_mat * in_normal
    );

}