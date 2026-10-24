#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::core {
	struct Window {
		void create(const Instance& instance, const Settings& info);
		void destroy(const Instance& instance);

		operator VK_TYPE(GLFWwindow*)() const;

		VK_TYPE(GLFWwindow*) window;
		VK_TYPE(VkSurfaceKHR) surface;
	};
}