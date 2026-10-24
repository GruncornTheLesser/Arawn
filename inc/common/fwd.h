#pragma once

#ifndef ARAWN_MAX_FRAMES
#define ARAWN_MAX_FRAMES 3
#endif

#ifndef ARAWN_MAX_IMAGES
#define ARAWN_MAX_IMAGES 8
#endif

#ifndef ARAWN_MAX_DEPTH_CHAIN
#define ARAWN_MAX_DEPTH_CHAIN 15
#endif

#ifndef ARAWN_INDEX_TYPE
#define ARAWN_INDEX_TYPE uint32_t
#endif

#ifndef ARAWN_VERSION_TYPE
#define ARAWN_VERSION_TYPE uint32_t
#endif


#include <cstdint>

namespace arawn::primitive {
	struct AABB;
	struct Cluster;
	struct MeshInstance;
	struct Plane;
	struct Tile;
	struct Vertex;
}

namespace arawn::asset {
	struct Mesh;
	struct Texture;
	struct Material;
}

namespace arawn::node {
	struct Node;
	struct Instance;
	struct Light;
	struct Camera;
}

namespace arawn::common {
	template<typename T> struct Handle;
	struct Pool;
}

namespace arawn::core {
	struct Settings;
	struct Instance;
	struct Window;
	struct Device;
	struct Swap;
	struct Frame;
}

namespace arawn::pool {
	struct Staging;
	struct Mesh;
	struct Light;
	struct Camera;
	struct Texture;
	struct Material;
}

namespace arawn::attachment {
	struct Color;
	struct Depth;
	struct ZTree;
	namespace cull {
		struct Light;
		struct Mesh;
	}
}

namespace arawn::pipeline::cull {
	struct Frustum;
	struct Light;
	struct Mesh;
	struct Occlusion;
}

namespace arawn::pipeline::gen {
	struct Cluster;
	struct Tile;
	struct ZTree;
}

namespace arawn::pipeline {
	struct DepthOnly;
	struct Forward;
}

namespace arawn::pass {
	struct EarlyCull;
	struct DepthOnly;
	struct LateCull;
	struct Forward;
	struct Present;
}

namespace arawn {
	template<class T>
	using Handle = common::Handle<T>;
}