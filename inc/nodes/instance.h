#pragma once
#include <common/fwd.h>
#include <common/handle.h>

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