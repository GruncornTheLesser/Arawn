#pragma once
#include "../common/vulkan.h"
#include "../primitive/vertex.h"
#include <span>
namespace arawn::assets {
	struct Mesh {
		struct CreateInfo {
			std::span<primitives::Vertex> vertices;
			std::span<uint32_t> indices;
		};

		VK_TYPE(VmaVirtualAllocation) vertices;
		VK_TYPE(VmaVirtualAllocation) indices;
		VK_TYPE(VmaVirtualAllocation) meshlets;
	};
}

namespace arawn::primitives {
	struct Mesh {
		uint32_t meshletOffset;
		uint32_t meshletCount;
	};
	struct Meshlet {
		uint32_t vertexOffset;
		uint32_t triangleOffset;
		uint32_t vertexCount;
		uint32_t triangleCount;
	};
}