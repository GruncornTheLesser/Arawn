#pragma once
#include <entt/entt.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/quaternion_float.hpp>

// components
struct hierarchy { 
    entt::entity parent; // cannot be null 
};
struct transform {
    glm::vec3 position;
    glm::vec3 scale;
    glm::quat rotation;
    glm::mat4 local;
    glm::mat4 world;
    struct update_tag { };
};

// scene
class scene : public entt::registry {
public:
    scene();

    void update_hierarchy();
    void update_transform();
};