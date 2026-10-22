#pragma once
#include <glm/glm.hpp>

namespace arawn::primitive {
	struct Material {
		glm::vec3 albedo;
		uint32_t albedoMap;
		float roughness;
		uint32_t roughnessMap;
		float metallic;
		uint32_t metallicMap;
		glm::vec3 emissive;
		uint32_t emissiveMap;
		float opacity = 1.0;
		uint32_t opacityMap;
		uint32_t normalMap;
		uint32_t bumpMap;
	};
}