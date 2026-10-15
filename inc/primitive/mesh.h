#pragma once
#include <glm/glm.hpp>


namespace arawn::primitives {
	struct Mesh {
		uint32_t meshletOffset;
		uint32_t meshletCount;
	};
}