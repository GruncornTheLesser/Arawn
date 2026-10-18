#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::engine {
	struct Core {
		void create(const Settings& info);
		void destroy();

		VK_TYPE(VkInstance) instance;
		#ifdef ARAWN_DEBUG
		VK_TYPE(VkDebugUtilsMessengerEXT) messenger;
		#endif
	};
}