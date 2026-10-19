#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::core {
	struct Instance {
		void create(const Settings& info);
		void destroy();

		operator VK_TYPE(VkInstance)() const;

		VK_TYPE(VkInstance) instance;
		#ifdef ARAWN_DEBUG
		VK_TYPE(VkDebugUtilsMessengerEXT) messenger;
		#endif
	};
}