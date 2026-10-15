#pragma once
#include <common/fwd.h>
#include <common/handle.h>
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