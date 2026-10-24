#pragma once
#include <common/vulkan.h>
#include <primitive/vertex.h>
#include <span>

namespace arawn::assets {
	struct Mesh {
		uint64_t vertexOffset, vertexSize;
		uint64_t indexOffset, indexSize;
	};
}