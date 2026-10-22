#pragma once
#include <glm/glm.hpp>


namespace arawn::primitive {
	struct Mesh {
		uint32_t meshletOffset;
		uint32_t meshletCount;
	};
}