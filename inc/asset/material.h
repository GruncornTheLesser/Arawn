#pragma once
#include <common/fwd.h>
#include <common/handle.h>
#include <glm/glm.hpp>

namespace arawn::asset {
	struct Material {		
		glm::vec3 albedo;
		float roughness;
		float metallic;
		Handle<asset::Texture> albedoMap;
		Handle<asset::Texture> normalMap;
		Handle<asset::Texture> metalRoughMap;
	};
}