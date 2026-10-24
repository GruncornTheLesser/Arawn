#pragma once
#include <glm/glm.hpp>

namespace arawn::primitive {
	struct Material { // 32
		glm::vec3 albedo;
		float roughness;
		float metallic;
		uint32_t albedoMap;
		uint32_t normalMap;
		uint32_t metalRoughMap;
	};
}