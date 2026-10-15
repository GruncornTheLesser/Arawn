#pragma once
#include <glm/glm.hpp>
#include <primitive/aabb.h>

namespace arawn::primitives {
    struct Meshlet {
		uint32_t vertexOffset;
		uint32_t faceOffset;
		uint32_t vertexCount;
		uint32_t faceCount;
        AABB aabb;
	};
}