#pragma once
#include <glm/glm.hpp>
namespace arawn::primitives {
	struct Vertex { // 24 bytes
		glm::vec3 position;
		uint32_t normal;	// A2R10G10B10_SNORM_PACK
		uint32_t tangent;
		uint16_t texcoord; // R8G8_UNORM

		Vertex(glm::vec3 position, glm::vec3 normal, glm::vec3 tangent, glm::vec2 texcoord);
	};
}