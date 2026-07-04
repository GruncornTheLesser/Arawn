#define ARAWN_INCLUDE_VULKAN
#define VMA_IMPLEMENTATION
#include <engine.h>
#include <vector>
#include <ranges>

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

template<typename ... Ts>
uint32_t passIndex(const Ts& ... ctxs) {
	uint32_t i = 0, w = 1;
	([&]{ i += ctxs.index * w; w *= ctxs.count; }(), ...);
	return i;
}

#if ARAWN_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugger(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* info, void* userData) {
	const char* typeName = "";
	switch(type) {
    	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: typeName = "general"; break;
    	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: typeName = "validation"; break;
    	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: typeName = "performance"; break;
    	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: typeName = "device_address_binding"; break;
		default: break;
	}
	
	switch (severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: ARAWN_LOG(INFO, info->pMessage) break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: ARAWN_LOG(WARNING, info->pMessage) break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: ARAWN_LOG(ERROR, info->pMessage) 
			throw "";
		break;
		default: break;
	}
	
	return VK_FALSE;
}
PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessengerEXT = nullptr;
#endif

Arawn::Engine::Engine(const AppInfo& app, const DisplayInfo& display) { 
	if (!glfwInit()) throw std::runtime_error("error: failed to init glfw");

	{ // create instance
		uint32_t vulkanVersion = VK_API_VERSION_1_3;
		
		ARAWN_LOG(INFO, std::format("application version={}.{}.{}", app.version.major, app.version.minor, app.version.patch));
		ARAWN_LOG(INFO, std::format("vulkan api version={}.{}.{}", VK_API_VERSION_MAJOR(vulkanVersion), VK_API_VERSION_MINOR(vulkanVersion), VK_API_VERSION_PATCH(vulkanVersion)));
		
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
		
		ARAWN_LOG(DEBUG, std::format("required instance extensions={}", requiredExts));
				
		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, 
			.pNext = nullptr, 
			.pApplicationName = app.title,
			.applicationVersion = 1,
			.pEngineName = "Arawn",
			.engineVersion = 1,
			.apiVersion = vulkanVersion
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
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = vulkanDebugger,
			.pUserData = nullptr
		};
		
		VK_ASSERT(createDebugUtilsMessengerEXT(instance, &info, nullptr, &messenger));
	}
#endif

	{ // window
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_REFRESH_RATE, display.refreshRate);
		
		// ARAWN_LOG(INFO, std::format("resolution=[{}, {}]", display.resolution.x, display.resolution.y));

		switch(display.mode) {
		case DisplayMode::WINDOWED: {
			glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
			glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
			
			window = glfwCreateWindow(
				display.resolution.x,
				display.resolution.y,
				app.title,
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
				app.title,
				nullptr,
				nullptr
			);
			break;
		}
		case DisplayMode::EXCLUSIVE: {
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
		
			window = glfwCreateWindow(
				display.resolution.x,
				display.resolution.y,
				app.title,
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
		VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface));

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

		if (display.gpu == nullptr || [&]{ 
			auto it = std::ranges::find_if(gpus, [&](const auto& gpu) { 
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpu, &properties);
				return strcmp(properties.deviceName, display.gpu);
			});
			
			if (it == gpus.end()) {
				ARAWN_LOG(WARNING, std::format("gpu \"{}\" not found.", display.gpu));
				return true;
			}

			if (unsupportedGPU(*it)) {
				ARAWN_LOG(WARNING, std::format("gpu \"{}\" not supported.", display.gpu));
				gpus.erase(it);
				return true;
			}
			
			gpu = *it;
			return false;
		}()) {
			std::erase_if(gpus, unsupportedGPU);
			ARAWN_ASSERT(!gpus.empty(), "no supported gpus found");
			
			gpu = *std::ranges::max_element(gpus, {}, scoreGPU);
		}
		
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(gpu, &properties);
		ARAWN_LOG(INFO, std::format("gpu=\"{}\"", properties.deviceName));
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
			
			assignQueue(VK_QUEUE_GRAPHICS_BIT, 0, queue.graphics);
			assignQueue(VK_QUEUE_COMPUTE_BIT, 0, queue.compute);
			assignQueue(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT, queue.transfer);
			assignPresentQueue(queue.present);
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
				float& queuePriority = ((queueInfos[input.family].pQueuePriorities - priorities.data()) + priorities.data())[input.index];
				queuePriority = std::max<float>(priority, queuePriority);
			};
		
			assignPriority(queue.graphics, 0.8);
			assignPriority(queue.compute, 0.6);
			assignPriority(queue.transfer, 0.4);
			assignPriority(queue.present, 1.0);
	
		}

		// ARAWN_LOG(INFO, std::format("graphics queue = {{ family = {}, index = {}, priority = {} }}", queue.graphics.family, queue.graphics.index, queueInfos[queue.graphics.family].pQueuePriorities[queue.graphics.index]));
		// ARAWN_LOG(INFO, std::format("compute queue = {{ family = {}, index = {}, priority = {} }}", queue.compute.family, queue.compute.index, queueInfos[queue.compute.family].pQueuePriorities[queue.compute.index]));
		// ARAWN_LOG(INFO, std::format("transfer queue = {{ family = {}, index = {}, priority = {} }}", queue.transfer.family, queue.transfer.index, queueInfos[queue.transfer.family].pQueuePriorities[queue.transfer.index]));
		// ARAWN_LOG(INFO, std::format("present queue = {{ family = {}, index = {}, priority = {} }}", queue.present.family, queue.present.index, queueInfos[queue.present.family].pQueuePriorities[queue.present.index]));

		// reduce queues -> loses family mapping
		std::erase_if(queueInfos, [](const auto& queueInfo) {
			return queueInfo.queueCount == 0;
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
		
		vkGetDeviceQueue(device, queue.graphics.family, queue.graphics.index, &queue.graphics.queue);
		vkGetDeviceQueue(device, queue.compute.family, queue.compute.index, &queue.compute.queue);
		vkGetDeviceQueue(device, queue.transfer.family, queue.transfer.index, &queue.transfer.queue);
		vkGetDeviceQueue(device, queue.present.family, queue.present.index, &queue.present.queue);
		
		{ // create descriptor pools
			VkCommandPoolCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
			};
	
			createInfo.queueFamilyIndex = queue.graphics.family;
			VK_ASSERT(vkCreateCommandPool(device, &createInfo, nullptr, &queue.graphics.pool));
	
			createInfo.queueFamilyIndex = queue.compute.family;
			VK_ASSERT(vkCreateCommandPool(device, &createInfo, nullptr, &queue.compute.pool));
	
			createInfo.queueFamilyIndex = queue.transfer.family;
			VK_ASSERT(vkCreateCommandPool(device, &createInfo, nullptr, &queue.transfer.pool));
	
			createInfo.queueFamilyIndex = queue.present.family;
			VK_ASSERT(vkCreateCommandPool(device, &createInfo, nullptr, &queue.present.pool));
		}
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
			.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.clipped = VK_TRUE,
			.oldSwapchain = nullptr,
		};

		createInfo.imageExtent = { display.resolution.x, display.resolution.y };
		if (capabilities.currentExtent.width != UINT32_MAX) {
			createInfo.imageExtent = {
				std::clamp(capabilities.currentExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width),
			 	std::clamp(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
			};
		}
		
		createInfo.minImageCount = std::max(display.buffering == BufferingMode::TRIPLE ? 3u : 2u, capabilities.minImageCount);
		if (capabilities.maxImageCount > 0) {
			createInfo.minImageCount = std::min(createInfo.minImageCount, capabilities.maxImageCount);
		}

		uint32_t presentModeCount;
		
		VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, nullptr));
		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, presentModes.data()));
		
		if (display.vsync == VsyncMode::ENABLED) {
			switch (display.latency) {
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
			switch (display.latency) {
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

		// ARAWN_LOG(INFO, std::format("swapchain present mode={}", string_VkPresentModeKHR(createInfo.presentMode)));

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

		// ARAWN_LOG(INFO, std::format("swapchain image format={}", string_VkFormat(createInfo.imageFormat)));
		
		VK_ASSERT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain))
	}

	{ // allocate resource context
		context.frame.index = 0;
		context.frame.count = display.buffering == BufferingMode::DOUBLE ? 2 : 3;
		context.frame.data = static_cast<Frame*>(cache.allocate(sizeof(Frame) * context.frame.count, alignof(Frame)));
		
		context.swap.index = 0;
		vkGetSwapchainImagesKHR(device, swapchain, &context.swap.count, nullptr);
		context.swap.data = static_cast<Swap*>(cache.allocate(sizeof(Swap) * context.swap.count, alignof(Swap)));
	}

	{ // init frame context
		{ // init sync primitves
			VkFenceCreateInfo fenceInfo{
    			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    			.pNext = nullptr,
    			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
			};

			VkSemaphoreCreateInfo semaInfo{
    			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    			.pNext = nullptr,
    			.flags = 0,
			};

			for (uint32_t i = 0; i < context.frame.count; ++i) {
				auto& ctx = context.frame.data[i];
				VK_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &ctx.inFlight));
				VK_ASSERT(vkCreateSemaphore(device, &semaInfo, nullptr, &ctx.imageAvailable));
				VK_ASSERT(vkCreateSemaphore(device, &semaInfo, nullptr, &ctx.forwardFinished));
			}
		}

		{ // color attachment
			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
			VkFormat format = findFormat({ 
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_FORMAT_B8G8R8A8_UNORM,
				VK_FORMAT_R8G8B8A8_SRGB,
			}, tiling, VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);

			std::array<uint32_t, 2> families = { queue.graphics.family, queue.present.family };
			VkImageCreateInfo imgInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = { display.resolution.x, display.resolution.y, 1 },
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				.sharingMode = VK_SHARING_MODE_CONCURRENT,
				.queueFamilyIndexCount = 2,
				.pQueueFamilyIndices = families.data(),
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			};

			if (queue.graphics.family == queue.present.family) {
				imgInfo.queueFamilyIndexCount = 1;
				imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}

			VmaAllocationCreateInfo allocInfo {
				.flags = 0,
				.usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.requiredFlags = 0,
				.preferredFlags = 0,
				.memoryTypeBits = 0,
				.pool = nullptr,
				.pUserData = nullptr,
				.priority = 0.0f,
			};

			VkImageViewCreateInfo viewInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = nullptr,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = format,
				.subresourceRange = { 
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			};

			for (uint32_t i = 0; i < context.frame.count; ++i) {
				auto& attachment = context.frame.data[i].colorAttachment;
				VK_ASSERT(vmaCreateImage(allocator, &imgInfo, &allocInfo, &attachment.image, &attachment.memory, nullptr));

				viewInfo.image = attachment.image;
				VK_ASSERT(vkCreateImageView(device, &viewInfo, nullptr, &attachment.view));
			}
		}

		{ // depth attachment
			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
			VkFormat format = findFormat({ 
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
			}, tiling, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

			std::array<uint32_t, 1> families = { queue.graphics.family };
			VkImageCreateInfo imgInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = { display.resolution.x, display.resolution.y, 1 },
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 1,
				.pQueueFamilyIndices = families.data(),
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			};

			VmaAllocationCreateInfo allocInfo {
				.flags = 0,
				.usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.requiredFlags = 0,
				.preferredFlags = 0,
				.memoryTypeBits = 0,
				.pool = nullptr,
				.pUserData = nullptr,
				.priority = 0.0f,
			};

			VkImageViewCreateInfo viewInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = nullptr,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = format,
				.subresourceRange = { 
					.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			};

			for (uint32_t i = 0; i < context.frame.count; ++i) {
				auto& attachment = context.frame.data[i].depthAttachment;
				VK_ASSERT(vmaCreateImage(allocator, &imgInfo, &allocInfo, &attachment.image, &attachment.memory, nullptr));

				viewInfo.image = attachment.image;
				VK_ASSERT(vkCreateImageView(device, &viewInfo, nullptr, &attachment.view));
			}
		}
	}

	{ // init swapchain context
		{ // init sync primitves
			VkSemaphoreCreateInfo semaInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    			.pNext = nullptr,
    			.flags = 0,
			};
			
			for (uint32_t i = 0; i < context.swap.count; ++i) {
				VK_ASSERT(vkCreateSemaphore(device, &semaInfo, nullptr, &context.swap.data[i].postprocessFinished));
			}
		}
		
		{ // init images
			std::vector<VkImage> images(context.swap.count);
			vkGetSwapchainImagesKHR(device, swapchain, &context.swap.count, images.data());
			
			for (uint32_t i = 0; i < images.size(); ++i) {
				context.swap.data[i].image = images[i];
			}
		}
	}


	struct DomainIndex { uint32_t index, count; };

	{ // init forward pass
		pass.forward.count = context.frame.count;

		VkCommandBufferAllocateInfo cmdInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = queue.graphics.pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = pass.forward.count,
		};
		
		std::vector<VkCommandBuffer> cmds(context.frame.count * context.swap.count);
		vkAllocateCommandBuffers(device, &cmdInfo, cmds.data());
		for (uint32_t frameIndex = 0; frameIndex < context.frame.count; ++frameIndex) {
			uint32_t index = passIndex(
				DomainIndex{ .index = frameIndex, .count = context.frame.count }
			);

			auto& pass = this->pass.forward.data[index];
			pass.cmd = cmds[index];

			pass.record(state, context.shared, context.frame.data[frameIndex]);
		}
	}

	{ // init present pass
		pass.present.count = context.frame.count * context.swap.count;

		VkCommandBufferAllocateInfo cmdInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = queue.present.pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = pass.present.count,
		};
		
		std::vector<VkCommandBuffer> cmds(pass.present.count);
		vkAllocateCommandBuffers(device, &cmdInfo, cmds.data());

		for (uint32_t frameIndex = 0; frameIndex < context.frame.count; ++frameIndex) for (uint32_t swapIndex = 0; swapIndex < context.swap.count; ++swapIndex) {
			uint32_t index = passIndex(
				DomainIndex{ .index = frameIndex, .count = context.frame.count }, 
				DomainIndex{ .index = swapIndex, .count = context.swap.count }
			);

			auto& pass = this->pass.present.data[index];
			pass.cmd = cmds[index];

			pass.record(state, context.shared, context.frame.data[frameIndex], context.swap.data[swapIndex]);
		}
	}
}

bool Arawn::Engine::closed() const {
	glfwPollEvents();
	return glfwWindowShouldClose(window);
}


Arawn::Engine::~Engine() noexcept {
	if (instance != nullptr) {
		vkDeviceWaitIdle(device);

		for (uint32_t i = 0; i < pass.forward.count; ++i) {
			auto& pass = this->pass.forward.data[i];
			vkFreeCommandBuffers(device, queue.graphics.pool, 1, &pass.cmd);
		}

		for (uint32_t i = 0; i < pass.present.count; ++i) {
			auto& pass = this->pass.present.data[i];
			vkFreeCommandBuffers(device, queue.present.pool, 1, &pass.cmd);
		}
		
		for (uint32_t i = 0; i < context.frame.count; ++i) {
			auto& ctx = context.frame.data[i];
			vkDestroySemaphore(device, ctx.imageAvailable, nullptr);
			vkDestroySemaphore(device, ctx.forwardFinished, nullptr);
			vkDestroyFence(device, ctx.inFlight, nullptr);

			vkDestroyImageView(device, ctx.colorAttachment.view, nullptr);
			vmaDestroyImage(allocator, ctx.colorAttachment.image, ctx.colorAttachment.memory);

			vkDestroyImageView(device, ctx.depthAttachment.view, nullptr);
			vmaDestroyImage(allocator, ctx.depthAttachment.image, ctx.depthAttachment.memory);
		}

		for (uint32_t i = 0; i < context.swap.count; ++i) {
			auto& ctx = context.swap.data[i];
			vkDestroySemaphore(device, ctx.postprocessFinished, nullptr);
		}
		
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		vmaDestroyAllocator(allocator);
		vkDestroyCommandPool(device, queue.graphics.pool, nullptr);
		vkDestroyCommandPool(device, queue.compute.pool, nullptr);
		vkDestroyCommandPool(device, queue.transfer.pool, nullptr);
		vkDestroyCommandPool(device, queue.present.pool, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		glfwDestroyWindow(window);
#if ARAWN_DEBUG
		destroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
#endif
		vkDestroyInstance(instance, nullptr);
	}
}

void Arawn::Engine::render() {
	Frame& frame = context.frame.data[context.frame.index];

	VK_ASSERT(vkWaitForFences(device, 1, &frame.inFlight, VK_TRUE, 1000000000)); // wait for frame ready
	
	VK_ASSERT(vkResetFences(device, 1, &frame.inFlight));

	{ // forward pass
		Forward& forwardPass = pass.forward.data[passIndex(context.frame)];

		std::array<VkSemaphore, 0> waits = { };
		std::array<VkPipelineStageFlags, 0> stages = { };
		std::array<VkSemaphore, 1> signals = { frame.forwardFinished };

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = static_cast<uint32_t>(waits.size()),
			.pWaitSemaphores = waits.data(),
			.pWaitDstStageMask = stages.data(),
			.commandBufferCount = 1,
			.pCommandBuffers = &forwardPass.cmd,
			.signalSemaphoreCount = static_cast<uint32_t>(signals.size()),
			.pSignalSemaphores = signals.data(),
		};
		
		VK_ASSERT(vkQueueSubmit(queue.graphics.queue, 1, &submitInfo, nullptr));
	}
	
	{ // acquire swapchain image
		VkResult res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, frame.imageAvailable, nullptr, &context.swap.index);
		if (res == VK_ERROR_OUT_OF_DATE_KHR) [[unlikely]] {
			// recreate swapchain
		} else if (res == VK_SUBOPTIMAL_KHR) [[unlikely]] {
			// recreate swapchain???
		} else VK_ASSERT(res);
	}
	Swap& swap = context.swap.data[context.swap.index];
	
	{ // post process pass
		Present& presentPass = pass.present.data[passIndex(context.frame, context.swap)];
		
		std::array<VkSemaphore, 2> waits = { frame.imageAvailable, frame.forwardFinished };
		std::array<VkPipelineStageFlags, 2> stages = { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
		std::array<VkSemaphore, 1> signals = { swap.postprocessFinished };
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = static_cast<uint32_t>(waits.size()),
			.pWaitSemaphores = waits.data(),
			.pWaitDstStageMask = stages.data(),
			.commandBufferCount = 1,
			.pCommandBuffers = &presentPass.cmd,
			.signalSemaphoreCount = static_cast<uint32_t>(signals.size()),
			.pSignalSemaphores = signals.data(),
		};
		
		VK_ASSERT(vkQueueSubmit(queue.present.queue, 1, &submitInfo, frame.inFlight));
	}
	
	{ // present 
		std::array<VkSemaphore, 1> waits = { swap.postprocessFinished };

		VkPresentInfoKHR submitInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = static_cast<uint32_t>(waits.size()),
			.pWaitSemaphores = waits.data(),
			.swapchainCount = 1,
			.pSwapchains = &swapchain,
			.pImageIndices = &context.swap.index,
			.pResults = nullptr,
		};

		VkResult res = vkQueuePresentKHR(queue.present.queue, &submitInfo);
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR /* or frame buffer resized */) {
			// recreate swapchain
		} else VK_ASSERT(res)
	}
	
	// increment contexts
	context.frame.index = (context.frame.index + 1) % context.frame.count;
}

void Arawn::Engine::Forward::record(const State& state, Shared& shared, Frame& frame) {
	{ // begin command buffer
		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr,
		};
		vkBeginCommandBuffer(cmd, &beginInfo);
	}

	{
		std::array<VkImageMemoryBarrier, 1> images = {
			VkImageMemoryBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = frame.colorAttachment.image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			}
		};

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 
			0, nullptr, 
			0, nullptr,
			static_cast<uint32_t>(images.size()), images.data()
		);
	}
	
	{ // clear op
		VkClearColorValue clear{ { 1.0f, 0.0f, 1.0f, 0.0f } };
		VkImageSubresourceRange range{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		};
		vkCmdClearColorImage(cmd, frame.colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);

	}

	vkEndCommandBuffer(cmd);
}

void Arawn::Engine::Present::record(const State& state, Shared& shared, Frame& frame, Swap& swap) {
	{ // begin command buffer
		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr,
		};
		vkBeginCommandBuffer(cmd, &beginInfo);
	}
	
	{ // barriers
		std::array<VkImageMemoryBarrier, 2> images = {
			VkImageMemoryBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = frame.colorAttachment.image,
				.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			},
			VkImageMemoryBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = swap.image,
				.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			},
		};
	
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 
			0, nullptr, 
			0, nullptr, 
			static_cast<uint32_t>(images.size()), images.data()); 
	}
	
	{ // copy op
		VkImageCopy region{
			.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			.srcOffset = { 0, 0, 0 },
			.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			.dstOffset = { 0, 0, 0 },
			.extent = { state.resolution.x, state.resolution.y, 1 },
		};
		vkCmdCopyImage(cmd, 
			frame.colorAttachment.image, 
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			swap.image, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			1, &region
		);
	}

	{ // barriers
		std::array<VkImageMemoryBarrier, 1> images = {
			VkImageMemoryBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = 0,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = swap.image,
				.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			},
		};
	
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 
			0, nullptr, 
			0, nullptr, 
			static_cast<uint32_t>(images.size()), images.data()); 
	}

	vkEndCommandBuffer(cmd);
}



VkFormat Arawn::Engine::findFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags required) const {
	for (VkFormat candidate : candidates) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(gpu, candidate, &properties);

		uint32_t flags = tiling == VK_IMAGE_TILING_LINEAR ? properties.linearTilingFeatures : properties.optimalTilingFeatures;
		if ((flags & required) != required) continue;

		return candidate;
	}

	throw std::runtime_error("failed to find format");
}