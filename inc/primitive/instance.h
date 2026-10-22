#pragma once
#include <glm/glm.hpp>
#include <common/handle.h>
#include <primitive/plane.h>

namespace arawn::primitive {
	struct Instance {
		Handle<uint32_t> node;
		Handle<uint32_t> mesh;
		Handle<uint32_t> material;
		uint32_t padding;
	};
}