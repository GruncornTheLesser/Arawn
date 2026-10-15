#pragma once
#include <glm/glm.hpp>
#include <primitive/plane.h>

namespace arawn::primitives {
	struct Instance {
		uint32_t node;
		uint32_t mesh;
		uint32_t material;
        uint32_t padding; // ?
	};
}