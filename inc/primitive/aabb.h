#pragma once
#include <glm/glm.hpp>


namespace arawn::primitives {
    struct AABB {
        glm::vec3 min, max;
    };
}