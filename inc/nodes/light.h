#pragma once
#include <common/fwd.h>
#include <common/handle.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace arawn::nodes {
	struct Light {
		enum Type : uint32_t { POINT, DIRECTIONAL, SPOT };

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