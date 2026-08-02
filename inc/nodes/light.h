#pragma once
#include "node.h"
#include "../assets/texture.h"

namespace arawn::nodes {
	struct Light {
		enum Type : uint32_t { POINT, DIRECTIONAL, SPOT };
		struct Primitive {

		};
		struct CreateInfo {
			Type type;
			Handle<Node> node;
			glm::vec3 color;
			float intensity;
			glm::vec3 position;
			float range;
			glm::vec3 direction;
			float innerCone;
			Handle<assets::Texture> shadowMap;
		};
		Handle<Node> node;
	};
}

namespace arawn::primitives {
	struct Light {
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