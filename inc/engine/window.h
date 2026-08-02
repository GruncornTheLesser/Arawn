#pragma once
#include "core.h"

namespace arawn::engine {
	struct Window {
		void create(const Core& core, const Settings& info);
		void recreate(const Core& core, const Settings& info);
		void destroy(const Core& core);

		VK_TYPE(GLFWwindow*) window;
		VK_TYPE(VkSurfaceKHR) surface;
	};
}