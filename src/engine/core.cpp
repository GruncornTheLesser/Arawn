#define VMA_IMPLEMENTATION
#define ARAWN_INCLUDE_VULKAN
#include <engine/core.h>
#include <vector>
#include <cstring>
#include <algorithm>


using namespace arawn;

constinit std::array instanceExtensions = std::to_array<const char*>({ 
#ifdef ARAWN_DEBUG
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
});

#ifdef ARAWN_DEBUG
constinit std::array debugLayers = std::to_array<const char*>({
	"VK_LAYER_KHRONOS_validation"
});
#endif

#ifdef ARAWN_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL vkMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* info, void* userData);
#endif

void engine::Core::create(const Settings& info) {
	if (!glfwInit()) throw std::runtime_error("error: failed to init glfw");
	
	ARAWN_LOG(INFO, std::format("{} version = {}.{}.{}.{}",
		info.title,
		info.version.variant,
		info.version.major,
		info.version.minor,
		info.version.patch)
	);
	ARAWN_LOG(INFO, std::format("arawn version = {}.{}.{}.{}", 
		VK_API_VERSION_VARIANT(ARAWN_VERSION),
		VK_API_VERSION_MAJOR(ARAWN_VERSION),
		VK_API_VERSION_MINOR(ARAWN_VERSION),
		VK_API_VERSION_PATCH(ARAWN_VERSION))
	);
	ARAWN_LOG(INFO, std::format("vulkan version = {}.{}.{}.{}", 
		VK_API_VERSION_VARIANT(ARAWN_VULKAN_VERSION),
		VK_API_VERSION_MAJOR(ARAWN_VULKAN_VERSION),
		VK_API_VERSION_MINOR(ARAWN_VULKAN_VERSION),
		VK_API_VERSION_PATCH(ARAWN_VULKAN_VERSION))
	);
	
	{ // create instance
		std::vector<const char*> extensions;
		
		uint32_t glfwExtCount;
		const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
		extensions.append_range(std::span{ glfwExts, glfwExts + glfwExtCount });
		extensions.append_range(instanceExtensions);
		
		std::ranges::sort(extensions, std::strcmp);
		extensions.erase(std::ranges::unique(extensions, [](const char* lhs, const char* rhs)->bool {
			return std::strcmp(lhs, rhs) == 0;
		}).end(), extensions.end());
		
		ARAWN_LOG(VERBOSE, std::format("instance extensions={}", extensions));
		
		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, 
			.pNext = nullptr, 
			.pApplicationName = info.title,
			.applicationVersion = VK_MAKE_API_VERSION(0, info.version.major, info.version.minor, info.version.patch),
			.pEngineName = "arawn",
			.engineVersion = ARAWN_VERSION,
			.apiVersion = ARAWN_VULKAN_VERSION
		};
	
		VkInstanceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &appInfo,
			#ifdef ARAWN_DEBUG
			.enabledLayerCount = static_cast<uint32_t>(debugLayers.size()),
			.ppEnabledLayerNames = debugLayers.data(),
			#endif
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data(),
		};
		
		VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &instance));
	}
	

	#ifdef ARAWN_DEBUG
	{ // create messenger
		PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		
		VkDebugUtilsMessengerCreateInfoEXT info{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = nullptr,
			.flags = 0, 
			.messageSeverity = 0,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = vkMessengerCallback,
			.pUserData = nullptr
		};
		
		#ifdef ARAWN_LOG_VERBOSE
		info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		#endif
	
		#ifdef ARAWN_LOG_WARNING
		info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		#endif
	
		#ifdef ARAWN_LOG_ERROR
		info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		#endif
	
		VK_ASSERT(createDebugUtilsMessengerEXT(instance, &info, nullptr, &messenger));
	}
	#endif
}

void engine::Core::destroy() {
	PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	
	destroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
	vkDestroyInstance(instance, nullptr);
}


#ifdef ARAWN_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL vkMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* info, void* userData) {
	const char* typeName = "";
	switch(type) {
    	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: typeName = "general"; break;
    	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: typeName = "validation"; break;
    	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: typeName = "performance"; break;
    	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: typeName = "device_address_binding"; break;
		default: break;
	}
	
	switch (severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: ARAWN_LOG(VERBOSE, info->pMessage) break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: ARAWN_LOG(INFO, info->pMessage) break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: ARAWN_LOG(WARNING, info->pMessage) break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: throw std::runtime_error(ARAWN_LOG_MESSAGE(ERROR, info->pMessage));
		default: break;
	}

	return VK_FALSE;
}
#endif