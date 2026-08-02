#pragma once
#include "../common/vulkan.h"
#include "../common/settings.h"

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