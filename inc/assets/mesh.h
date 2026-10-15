#pragma once
#include <common/vulkan.h>
#include <primitive/vertex.h>
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