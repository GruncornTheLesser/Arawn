#pragma once
#include <glm/glm.hpp>
#include <primitive/plane.h>

namespace arawn::primitives {
    struct Cluster {
        struct Header { 
            Plane planes[6];
            uint32_t count;
        } header;
        uint32_t indices[];
    };
}