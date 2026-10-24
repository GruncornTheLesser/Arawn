#pragma once 
#include <common/handle.h>
#include <glm/glm.hpp>

namespace arawn::node {
    struct Light {
        glm::vec3 color;
		uint32_t node; // transform index lookup
		float intensity; // fall off curve
		float range; // range = 0 -> directional light
		float cone; // inner and outer cone size 16bit floats
		uint32_t shadow; // shadow map
    };
}