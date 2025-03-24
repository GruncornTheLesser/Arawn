#pragma once
#include "vulkan.h"

struct Vertex { 
    enum Attribute : uint32_t { POSITION = 1, TEXCOORD = 2, NORMAL = 4, TANGENT = 8 };
    glm::vec3 position;
    glm::vec2 texcoord;
    glm::vec3 normal; 
    glm::vec4 tangent; // tangent(x, y) + bi-tangent(x, y)

    bool operator==(const Vertex& other) const {
        return position == other.position && 
               texcoord == other.texcoord && 
               normal == other.normal;
    }

};