#pragma once
#include <glm/glm.hpp>


namespace arawn::primitive {
    struct Tile {
        struct Header {
            float min, max;
        } headers;
        uint32_t indices[];
    };
}