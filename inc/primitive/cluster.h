#pragma once
#include <glm/glm.hpp>
#include <primitive/plane.h>

namespace arawn::primitive {
    struct Cluster {
        Plane planes[6];
        uint32_t count;
        uint32_t indices[];
    };
}