#pragma once
#include <common/fwd.h>
#include <common/handle.h>
#include <glm/glm.hpp>

namespace arawn::asset {
	struct Material {		
		glm::vec3 albedo;
		float roughness;
		float metallic;
		uint32_t albedoMap;
		uint32_t normalMap;
		uint32_t metalRoughMap;
	};
}