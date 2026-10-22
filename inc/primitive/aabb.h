#pragma once
#include <glm/glm.hpp>


namespace arawn::primitive {
    struct AABB {
        glm::vec3 min, max;
    };
}