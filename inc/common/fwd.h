#pragma once

namespace arawn::common {
	template<typename T> struct Handle;
	template<typename T> struct Pool;
	template<typename T> struct Flag;
	struct Settings;
}

namespace arawn::resources {
	struct Buffer;
	struct Image;
	struct Pipeline;
}

namespace arawn::primitives {
	struct Meshlet;
	struct Vertex;
	struct Camera;
}

namespace arawn {
	class Engine;
}

namespace arawn::engine {
	struct Core;
	struct Window;
	struct Device;
	struct Swap;
	struct World;
	struct Render;
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
	using Settings = common::Settings;
}