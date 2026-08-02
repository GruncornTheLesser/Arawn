#pragma once
#include "../common/handle.h"
#include "texture.h"
#include <glm/glm.hpp>

namespace arawn::assets {
	struct Material {
		struct CreateInfo {
			glm::vec3 albedo;
			Handle<Texture> albedoMap;
			float roughness;
			Handle<Texture> roughnessMap;
			float metallic;
			Handle<Texture> metallicMap;
			glm::vec3 emissive = { 0.0, 0.0, 0.0 };
			Handle<Texture> emissiveMap;
			float opacity = 1.0;
			Handle<Texture> opacityMap;
			Handle<Texture> normalMap;
			Handle<Texture> bumpMap;
		};
	};
}

namespace arawn::primitives {
	struct Material {
		glm::vec3 albedo;
		uint32_t albedoMap;
		float roughness;
		uint32_t roughnessMap;
		float metallic;
		uint32_t metallicMap;
		glm::vec3 emissive = { 0.0, 0.0, 0.0 };
		uint32_t emissiveMap;
		float opacity = 1.0;
		uint32_t opacityMap;
		uint32_t normalMap;
		uint32_t bumpMap;
	};
}