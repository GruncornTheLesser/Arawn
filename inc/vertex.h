#pragma once
#include "vulkan.h"

struct Vertex { 
    glm::vec3 position;
    glm::vec2 texcoord;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bi_tangent;

    bool operator==(const Vertex& other) const {
        return position == other.position && 
               texcoord == other.texcoord && 
               normal == other.normal;
    }

};