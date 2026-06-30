#define VMA_IMPLEMENTATION
#include <engine.h>
#include <vector>
#include <ranges>
#include <format>

const std::vector<const char*> instanceExtensions = {
#if ARAWN_DEBUG
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
};

const std::vector<const char*> debugLayers {
#if ARAWN_DEBUG
	"VK_LAYER_KHRONOS_validation"
#endif
};

#if ARAWN_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::printf("%s\n", pCallbackData->pMessage);
	return VK_FALSE;
}
PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessengerEXT = nullptr;
#endif


Arawn::Engine::Engine(const Info& info) { 
	if (!glfwInit()) throw std::runtime_error("error: failed to init glfw");

	{ // create instance
		std::vector<const char*> requiredExts = instanceExtensions;
		uint32_t glfwExtCount;
		const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
		requiredExts.insert(requiredExts.end(), glfwExts, glfwExts + glfwExtCount);
		
		std::ranges::sort(requiredExts, [](const char* lhs, const char* rhs)->bool {
			return std::strcmp(lhs, rhs);
		});
		
		requiredExts.erase(std::ranges::unique(requiredExts, [](const char* lhs, const char* rhs)->bool {
			return std::strcmp(lhs, rhs) == 0;
		}).end(), requiredExts.end());
	
	
		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, 
			.pNext = nullptr, 
			.pApplicationName = info.appName,
			.applicationVersion = 1,
			.pEngineName = "Arawn",
			.engineVersion = 1,
			.apiVersion = VK_API_VERSION_1_3
		};
		VkInstanceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pApplicationInfo = &appInfo,
	#if ARAWN_DEBUG
			.enabledLayerCount = static_cast<uint32_t>(debugLayers.size()),
			.ppEnabledLayerNames = debugLayers.data(),
	#endif
			.enabledExtensionCount = static_cast<uint32_t>(requiredExts.size()),
			.ppEnabledExtensionNames = requiredExts.data(),
		};
		
		VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &instance))
	}

#if ARAWN_DEBUG
	{ // create debug messenger
		if (createDebugUtilsMessengerEXT == nullptr) {
			createDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			destroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		}

		VkDebugUtilsMessengerCreateInfoEXT info{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = nullptr,
			.flags = 0, 
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debugCallback,
			.pUserData = nullptr
		};
		
		VK_ASSERT(createDebugUtilsMessengerEXT(instance, &info, nullptr, &messenger));
	}
#endif

	{ // window
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_REFRESH_RATE, info.refreshRate);
	
		switch(info.mode) {
		case DisplayMode::WINDOWED: {
			glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
			glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
	
			window = glfwCreateWindow(
				info.resolution.x,
				info.resolution.y,
				info.appName,
				nullptr,
				nullptr
			);
			break;
		}
		case DisplayMode::FULLSCREEN: {
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);
			
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
	
			window = glfwCreateWindow(
				vidmode->width, 
				vidmode->height,
				info.appName,
				nullptr,
				nullptr
			);
			break;
		}
		case DisplayMode::EXCLUSIVE: {
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
		
			window = glfwCreateWindow(
				info.resolution.x,
				info.resolution.y,
				info.appName,
				glfwGetPrimaryMonitor(),
				nullptr
			);
			break;
		}	
		}
	
		if (!window) [[unlikely]] {
			const char* message;
			glfwGetError(&message);
	
			std::printf("glfw error: %s\n", message);
			throw std::runtime_error("");
		}

		VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface))
	}

	{ // select gpu
		auto unsupportedGPU = [&](VkPhysicalDevice gpu) { 
			// properties
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(gpu, &properties);
			
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
				return true;
			}
			
			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, families.data());

			// queue requirements
			if (std::ranges::none_of(families, [](const auto& family)->bool { 
				return (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT;
			})) return true;
	
			if (std::ranges::none_of(families, [](const auto& family)->bool { 
				return (family.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT;
			})) return true;
	
			if (std::ranges::none_of(families, [](const auto& family)->bool { 
				return (family.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT;
			})) return true;
	
			if (std::ranges::none_of(std::views::iota(static_cast<uint32_t>(0), static_cast<uint32_t>(families.size())), [&](uint32_t i)->bool { 
				VkBool32 supported;
				vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &supported);
				return supported == VK_TRUE;
			})) return true;
	
			// extensions
			uint32_t extCount;
			VK_ASSERT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extCount, nullptr));
			std::vector<VkExtensionProperties> extensions(extCount);
			VK_ASSERT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extCount, extensions.data()));

			for (const char* ext : deviceExtensions) {
				if (std::ranges::none_of(extensions, [ext](const auto& candidate) {
					return strcmp(candidate.extensionName, ext);
				})) return true;
			}
	
			// features
			// auto features = gpu.getFeatures();
	
			return false;
		};
	
		auto scoreGPU = [&](VkPhysicalDevice gpu)->uint32_t {
			uint32_t score = 0;
		
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(gpu, &properties);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				score += 10000;
			}
			
			VkPhysicalDeviceMemoryProperties memory;
			vkGetPhysicalDeviceMemoryProperties(gpu, &memory);
			
			for (const auto& heap : memory.memoryHeaps) {
				if (heap.flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
					score += (heap.size / (1024 * 1024 * 1024)) * 50;
				}
			}
			
			// VkPhysicalDeviceFeatures features;
			// vkGetPhysicalDeviceFeatures(gpu, &features);

			return score;
		};
		
		uint32_t gpuCount;
		VK_ASSERT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
		std::vector<VkPhysicalDevice> gpus(gpuCount);
		VK_ASSERT(vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data()));

		if (info.gpu == nullptr || [&]{ 
			auto it = std::ranges::find_if(gpus, [&](const auto& gpu) { 
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpu, &properties);
				return strcmp(properties.deviceName, info.gpu);
			});
			
			if (it == gpus.end()) {
				ARAWN_LOG("device" << info.gpu << "not found...");
				return true;
			}

			if (unsupportedGPU(*it)) {
				ARAWN_LOG("device" << info.gpu << "not supported...");
				gpus.erase(it);
				return true;
			}
			gpu = *it;
			return false;
		}()) {
			std::erase_if(gpus, unsupportedGPU);
			ARAWN_ASSERT(!gpus.empty(), "no supported gpus found");
			
			gpu = *std::ranges::max_element(gpus, {}, scoreGPU);
			
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(gpu, &properties);
			ARAWN_LOG(properties.deviceName);
		}
	}

	{ // select device
		uint32_t familyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
		std::vector<VkQueueFamilyProperties> families(familyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families.data());
		
		std::vector<VkDeviceQueueCreateInfo> queueInfos(families.size(), VkDeviceQueueCreateInfo{ 
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		});
		for (uint32_t i = 0; i < families.size(); ++i) queueInfos[i].queueFamilyIndex = i;

		{ // assign queue family and index
			auto assignQueue = [&](VkQueueFlags required, VkQueueFlags excluded, Queue& output) {
				for (uint32_t i = 0; i < families.size(); ++i) {
					auto& family = families[i];
					auto& queueCount = queueInfos[i].queueCount;
					
					if ((family.queueFlags & required) != required) continue;
					if ((family.queueFlags & excluded) != 0) continue;
					if (queueCount == family.queueCount) continue;
					
					output.family = i;
					output.index = queueCount++;
					return;
				}
	
				for (uint32_t i = 0; i < families.size(); ++i) {
					auto& family = families[i];
					
					if (!(family.queueFlags & required)) continue;
	
					output.family = i;
					output.index = 0;
					return;
				}
	
				ARAWN_ASSERT(false, "no suitable queue found on selected device.");
			};
	
			auto assignPresentQueue = [&](Queue& output) {
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpu, &properties);
				
				for (uint32_t i = 0; i < families.size(); ++i) {
					auto& family = families[i];
					
					VkBool32 supported;
					VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &supported));
					if (supported == VK_FALSE) continue;
					if (!(family.queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;
	
					output.family = i;
					output.index = 0;
	
					if (family.queueCount == 0) ++family.queueCount;
					
					return;
				}
	
				throw std::runtime_error("error: no suitable queue found on selected device.");
			};
			
			assignQueue(VK_QUEUE_GRAPHICS_BIT, 0, graphics);
			assignQueue(VK_QUEUE_COMPUTE_BIT, 0, compute);
			assignQueue(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT, transfer);
			// requestQueue(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT, async);
			assignPresentQueue(present);
	
		}
	
		// assign priorities
		std::vector<float> priorities(std::ranges::fold_left(queueInfos, 0, [](uint32_t count, VkDeviceQueueCreateInfo& family) {
			return count + family.queueCount;
		}));
		{
			float* priority = priorities.data();
			for (auto& family : queueInfos) {
				family.pQueuePriorities = priority;
				priority += family.queueCount;
			}
		
			auto assignPriority = [&](const Queue& input, float priority) {
				std::max(priority, ((queueInfos[graphics.family].pQueuePriorities - priorities.data()) + priorities.data())[graphics.index]);
			};
		
			assignPriority(graphics, 0.8);
			assignPriority(compute, 0.6);
			assignPriority(transfer, 0.4);
			assignPriority(present, 1.0);
	
		}
	
		// reduce queues -> loses family mapping
		std::erase_if(queueInfos, [](const auto& info) {
			return info.queueCount == 0;
		});

		VkPhysicalDeviceFeatures features{ };
		VkDeviceCreateInfo info {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
			.pQueueCreateInfos = queueInfos.data(),
			.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = &features,
		};
		VK_ASSERT(vkCreateDevice(gpu, &info, nullptr, &device))
		
		vkGetDeviceQueue(device, graphics.family, graphics.index, &graphics.queue);
		vkGetDeviceQueue(device, compute.family, compute.index, &compute.queue);
		vkGetDeviceQueue(device, transfer.family, transfer.index, &transfer.queue);
		vkGetDeviceQueue(device, present.family, present.index, &present.queue);
	}

	{ // create allocator
		VmaAllocatorCreateInfo createInfo = {
			.flags = 0,
			.physicalDevice = gpu,
			.device = device,
			.instance = instance,
			.vulkanApiVersion = VK_API_VERSION_1_3,
		};
		VK_ASSERT(vmaCreateAllocator(&createInfo, &allocator));
	}

	{ // create swapchain
		
		VkSurfaceCapabilitiesKHR capabilities;
		VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capabilities));
	
		VkSwapchainCreateInfoKHR createInfo{ 
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = surface,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.clipped = VK_TRUE,
			.oldSwapchain = nullptr,
		};

		createInfo.imageExtent = { info.resolution.x, info.resolution.y };
		if (capabilities.currentExtent.width != UINT32_MAX) {
			createInfo.imageExtent = {
				std::clamp(capabilities.currentExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width),
			 	std::clamp(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
			};
		}
		
		createInfo.minImageCount = std::max(info.buffering == BufferingMode::TRIPLE ? 3u : 2u, capabilities.minImageCount);
		if (capabilities.maxImageCount > 0) {
			createInfo.minImageCount = std::min(createInfo.minImageCount, capabilities.maxImageCount);
		}

		uint32_t presentModeCount;
		VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, nullptr));
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, presentModes.data()));
		
		if (info.vsync == VsyncMode::ENABLED) {
			switch (info.latency) {
			case LowLatencyMode::ENABLED:
			case LowLatencyMode::BALANCED:
				if (std::ranges::contains(presentModes, VK_PRESENT_MODE_MAILBOX_KHR)) {
					createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
			case LowLatencyMode::DISABLED:
				createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
				break;
			}
		} else {
			switch (info.latency) {
			case LowLatencyMode::ENABLED:
				if (std::ranges::contains(presentModes, VK_PRESENT_MODE_IMMEDIATE_KHR)) {
					createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
					break;
				}
			case LowLatencyMode::BALANCED:
			case LowLatencyMode::DISABLED:
				if (std::ranges::contains(presentModes, VK_PRESENT_MODE_FIFO_RELAXED_KHR)) {
					createInfo.presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
					break;
				}
				
				createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
				break;
			}
		}

		uint32_t formatCount;
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr));
		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, surfaceFormats.data()));
		
		createInfo.imageFormat = surfaceFormats.front().format;
		createInfo.imageColorSpace = surfaceFormats.front().colorSpace;
		for (const auto& candidate : surfaceFormats) { // if preferred format
			switch (candidate.format)
			{
				case(VK_FORMAT_R8G8B8A8_SRGB): break;
				case(VK_FORMAT_B8G8R8A8_SRGB): break;
				case(VK_FORMAT_R8G8B8A8_UNORM): break;
				case(VK_FORMAT_B8G8R8A8_UNORM): break;
				default: continue;
			}

			createInfo.imageFormat = candidate.format;
			createInfo.imageColorSpace = candidate.colorSpace;
		}

		VK_ASSERT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain))
	}
}

Arawn::Engine::~Engine() {
	if (instance != nullptr) {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		vmaDestroyAllocator(allocator);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		glfwDestroyWindow(window);
#if ARAWN_DEBUG
		destroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
#endif
		vkDestroyInstance(instance, nullptr);
	}
}

Arawn::Engine::Engine(Engine&& other) {
	instance = other.instance;
	#if ARAWN_DEBUG
	messenger = other.messenger;
	#endif
	
	 window = other.window;
	surface = other.surface;
	
	gpu = other.gpu;
	graphics = other.graphics;
	compute = other.compute;
	transfer = other.transfer;
	present = other.present;
	device = other.device;
	allocator = other.allocator;
	
	surfaceFormat = other.surfaceFormat;
	swapchain = other.swapchain;

	other.instance = nullptr;
}

Arawn::Engine& Arawn::Engine::operator=(Engine&& other) {
	if (instance != nullptr) {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		vmaDestroyAllocator(allocator);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		glfwDestroyWindow(window);
#if ARAWN_DEBUG
		destroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
#endif
		vkDestroyInstance(instance, nullptr);
	}

	instance = other.instance;
	#if ARAWN_DEBUG
	messenger = other.messenger;
	#endif
	window = other.window;
	surface = other.surface;
	gpu = other.gpu;
	graphics = other.graphics;
	compute = other.compute;
	transfer = other.transfer;
	present = other.present;
	device = other.device;
	allocator = other.allocator;
	surfaceFormat = other.surfaceFormat;
	swapchain = other.swapchain;
	other.instance = nullptr;

	return *this;
}