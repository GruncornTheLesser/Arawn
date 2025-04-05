#version 450

vec2 quad[6] = { 
    vec2(0.0, 0.0),
    vec2(0.0, 0.0),
    vec2(0.0, 0.0),
    vec2(0.0, 0.0), 
    vec2(0.0, 0.0), 
    vec2(0.0, 0.0)
};

void main() {
    gl_Position = vec4(quad[gl_VertexIndex], 0, 1.0);
}