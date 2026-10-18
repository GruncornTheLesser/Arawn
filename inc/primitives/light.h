#pragma once
#include <glm/glm.hpp>


namespace arawn::primitives {
    struct Light { // 64 bytes
		uint32_t type;
		uint32_t node;
		glm::vec3 color;
		float intensity;
		glm::vec3 position;
		float range;
		glm::vec3 direction;
		float innerCone;
		float outerCone;
		uint32_t shadowMap;
	};
}