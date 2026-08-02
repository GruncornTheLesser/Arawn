#pragma once
#include "../common/handle.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace arawn::nodes {
	struct Node { 
		enum UpdateFlags {
			POSITION = 1,
			ROTATION = 2,
			SCALE = 4,
		};
		struct CreateInfo {
			Handle<Node> parent;
			glm::vec3 position;
			glm::quat rotation;
			float scale;
		};

		glm::mat4 local;
		glm::mat4 world;
		Handle<Node> parent;
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
		uint8_t updateFlags;
	};
}

namespace arawn::primitives {
	struct Node { 
		glm::mat4 transform;
	};
}