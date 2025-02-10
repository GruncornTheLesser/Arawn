#version 450

layout(location = 0) in vec3 col;
layout(location = 0) out vec4 fin;

void main() {
    fin = vec4(col, 1.0);
}