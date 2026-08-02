#pragma once
#include <glm/glm.hpp>
namespace arawn::primitives {
	struct Vertex { // 32 bytes
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texcoord;
	};
}