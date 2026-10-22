#pragma once

namespace arawn::common {
	template<typename T> struct Handle;
	template<typename T> struct Pool;
	template<typename T> struct Flag;
}

namespace arawn::primitive {
	struct AABB;
	struct Camera;
	struct Cluster;
	struct MeshInstance;
	struct Light;
	struct Material;
	struct Mesh;
	struct Meshlet;
	struct Plane;
	struct Tile;
	struct Vertex;
}

namespace arawn::core {
	struct Settings;
	struct Instance;
	struct Window;
	struct Device;
	struct Swap;
	struct Frame;
}

namespace arawn::image {
	struct Color;
	struct Depth;
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

namespace arawn::node {
	struct Node;
	struct Instance;
	struct Light;
	struct Camera;
}

namespace arawn::asset {
	struct Mesh;
	struct Texture;
	struct Material;
}

namespace arawn::pool {
	struct Mesh;
	struct Light;
	struct Camera;
	struct Texture;
	struct Material;
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