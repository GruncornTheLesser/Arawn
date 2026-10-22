#pragma once
#include <glm/glm.hpp>
#include <primitive/plane.h>

namespace arawn::primitive {
    struct Camera {
        glm::mat4 view;
        glm::vec3 eye;
        float fov;
        Plane planes[6];
    };
}