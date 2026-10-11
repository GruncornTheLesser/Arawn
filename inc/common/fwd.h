#pragma once

namespace arawn::common {
	template<typename T> struct Handle;
	template<typename T> struct Pool;
	template<typename T> struct Flag;
	struct Settings;
}

namespace arawn::primitive {
	struct Meshlet;
	struct Vertex;
}

namespace arawn {
	class Engine;
}

namespace arawn::engine {
	struct Core;
	struct Device;
	struct Present;
	struct Render;
	struct Swap;
	struct Window;
	struct World;
}

namespace arawn::assets {
	struct Mesh;
	struct Texture;
	struct Material;
}

namespace arawn::nodes {
	struct Node;
	struct Instance;
	struct Light;
	struct Camera;
}

namespace arawn {
	template<class T>
	using Handle = common::Handle<T>;
}