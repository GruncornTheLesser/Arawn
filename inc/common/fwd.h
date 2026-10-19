#pragma once

namespace arawn::common {
	template<typename T> struct Handle;
	template<typename T> struct Pool;
	template<typename T> struct Flag;
}

namespace arawn::attachments {
	struct Mesh;
	struct Depth;
	struct Color;
}

namespace arawn::pools {
	struct Mesh;
	struct Light;
	struct Camera;
	struct Texture;
	struct Material;
}

namespace arawn::primitives {
	struct Meshlet;
	struct Vertex;
	struct Camera;
}

namespace arawn::core {
	struct Settings;
	struct Instance;
	struct Window;
	struct Device;
	struct Swap;
	struct Frame;
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