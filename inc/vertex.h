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
               normal == other.normal && 
               tangent == other.tangent;
    }

};

namespace std { // hash for vertex
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return hash<glm::vec3>()(vertex.position) ^ 
                hash<glm::vec2>()(vertex.texcoord) ^ 
                hash<glm::vec2>()(vertex.normal) ^ 
                hash<glm::vec2>()(vertex.tangent);
        }
    };
}