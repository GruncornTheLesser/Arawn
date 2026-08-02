#pragma once
#include "node.h"
#include "../assets/mesh.h"
#include "../assets/material.h"

namespace arawn::nodes {
	struct Instance {
		struct CreateInfo {
			Handle<nodes::Node> node;
			Handle<assets::Mesh> mesh;
			Handle<assets::Material> material;
		};
		
		Handle<nodes::Node> node;
		Handle<assets::Mesh> mesh;
		Handle<assets::Material> material;
	};
}

namespace arawn::primitives {
	struct Instance {
		uint32_t node;
		uint32_t mesh;
		uint32_t material;
		uint32_t skeleton;
	};
};