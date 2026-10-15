#pragma once
#include <glm/glm.hpp>


namespace arawn::primitives {
    struct Tile {
        struct Header {
            float min, max;
        } headers;
        uint32_t indices[];
    };
}