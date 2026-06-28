#define VMA_IMPLEMENTATION
#include <engine.h>
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

Arawn::Engine::Engine(const Settings& settings) 
 : settings(settings)
 , instance(createInstance())
 #if ARAWN_DEBUG
 , debugger(createDebugMessenger())
 #endif
 , window(createWindow())
 , surface(createSurface())
 , gpu(selectGPU())
 , device(createDevice())
 , allocator(createAllocator())
 , swapchain(createSwapchain()) {
	
}

vk::raii::Instance Arawn::Engine::createInstance() {
	if (!glfwInit()) throw std::runtime_error("error: failed to init glfw");

	vk::ApplicationInfo app_info(settings.app.appName, settings.app.appVersion, "Arawn", 1, VK_API_VERSION_1_3);
	vk::InstanceCreateInfo info({}, &app_info);
	
	std::vector<const char*> required = instanceExtensions;
	uint32_t glfwExtCount;
	const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
	required.insert(required.end(), glfwExts, glfwExts + glfwExtCount);
	
	std::ranges::sort(required, [](const char* lhs, const char* rhs)->bool {
		return std::strcmp(lhs, rhs);
	});
	
	required.erase(std::ranges::unique(required, [](const char* lhs, const char* rhs)->bool {
		return std::strcmp(lhs, rhs) == 0;
	}).end(), required.end());

	info.setPEnabledExtensionNames(required);
	info.setPEnabledLayerNames(debugLayers);
	
	return vk::raii::Instance(context, info);
}

#if ARAWN_DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageType, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	const char* severity;
	switch (messageSeverity) {
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: 
		severity = "verbose";
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: 
		severity = "info";
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: 
		severity = "warning";
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: 
		severity = "error";
		break;
	}

	std::printf("[vulkan %s]: %s\n", severity, pCallbackData->pMessage);
	return VK_FALSE;
}

vk::raii::DebugUtilsMessengerEXT Arawn::Engine::createDebugMessenger() {
    vk::DebugUtilsMessengerCreateInfoEXT info{
		{}, 
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
		| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		debugCallback,
		nullptr
	};
	
	return instance.createDebugUtilsMessengerEXT(info);
}
#endif

Arawn::Engine::WindowRAII Arawn::Engine::createWindow() {
	GLFWwindow* window;

	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_REFRESH_RATE, settings.display.refreshRate);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	switch(settings.display.mode) {
	case DisplayMode::WINDOWED: {
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);

		window = glfwCreateWindow(
			settings.display.resolution.x,
			settings.display.resolution.y,
			settings.app.appName,
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
			settings.app.appName,
			nullptr,
			nullptr
		);
		break;
	}
	case DisplayMode::EXCLUSIVE: {
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
	
		window = glfwCreateWindow(
			settings.display.resolution.x,
			settings.display.resolution.y,
			settings.app.appName,
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
	return WindowRAII(window);
}

vk::raii::SurfaceKHR Arawn::Engine::createSurface() {
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(*instance, window.get(), nullptr, &surface) != VK_SUCCESS) {
		const char* message;
		glfwGetError(&message);
		std::printf("glfw error: %s\n", message);
		throw std::runtime_error("failed to create surface");
	}
	return vk::raii::SurfaceKHR(instance, surface);
}

vk::raii::PhysicalDevice Arawn::Engine::selectGPU() {
	auto unsupportedGPU = [&](vk::PhysicalDevice gpu) { 
		// properties
		auto properties = gpu.getProperties();
		if (properties.deviceType == vk::PhysicalDeviceType::eCpu) {
			return true;
		}
		
		auto families = gpu.getQueueFamilyProperties();
		// queue requirements
		if (std::ranges::none_of(families, [](const auto& family)->bool { 
			return (family.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics;
		})) return true;

		if (std::ranges::none_of(families, [](const auto& family)->bool { 
			return (family.queueFlags & vk::QueueFlagBits::eCompute) == vk::QueueFlagBits::eCompute;
		})) return true;

		if (std::ranges::none_of(families, [](const auto& family)->bool { 
			return (family.queueFlags & vk::QueueFlagBits::eTransfer) == vk::QueueFlagBits::eTransfer;
		})) return true;

		if (std::ranges::none_of(std::views::iota(static_cast<uint32_t>(0), static_cast<uint32_t>(families.size())), [&](uint32_t i)->bool { 
			return gpu.getSurfaceSupportKHR(i, surface);
		})) return true;

		// extensions
		auto extensions = gpu.enumerateDeviceExtensionProperties();
		for (const char* ext : deviceExtensions) {
			if (std::ranges::none_of(extensions, [ext](const auto& candidate) {
				return strcmp(candidate.extensionName, ext);
			})) return true;
		}

		// features
		// auto features = gpu.getFeatures();

		return false;
	};

	auto scoreGPU = [&](vk::PhysicalDevice gpu)->uint32_t {
		uint32_t score = 0;
		auto props = gpu.getProperties();
		auto features = gpu.getFeatures();

		if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			score += 10000;
		}

		auto memProps = gpu.getMemoryProperties();
		for (const auto& heap : memProps.memoryHeaps) {
			if (heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
				score += (heap.size / (1024 * 1024 * 1024)) * 50;
			}
		}

		return score;
	};

	auto gpus = instance.enumeratePhysicalDevices();
	
	if (settings.display.gpu == nullptr) {
		std::printf("no device selected...\n");
	} else if (auto it = std::ranges::find_if(gpus, [&](const auto& gpu) { 
		return strcmp(gpu.getProperties().deviceName, settings.display.gpu);
	}); it == gpus.end()) {
		std::printf("device \"%s\" not found...\n", settings.display.gpu);
	} else if (unsupportedGPU(*it)) {
		std::printf("device \"%s\" not suitable...\n", settings.display.gpu);
		gpus.erase(it);
	} else {
		return *it;
	}
	
	std::erase_if(gpus, unsupportedGPU);
	if (gpus.empty()) throw std::runtime_error("no supported gpus found");
	
	auto selected = *std::ranges::max_element(gpus, {}, scoreGPU);
	
	settings.display.gpu = selected.getProperties().deviceName;	
	std::printf("device \"%s\" selected\n", settings.display.gpu);
	
	return selected;
}

vk::raii::Device Arawn::Engine::createDevice() {	
	vk::DeviceCreateInfo info;
	vk::PhysicalDeviceFeatures features{ };
	auto families = gpu.getQueueFamilyProperties();
	
	std::vector<vk::DeviceQueueCreateInfo> queueInfos(families.size());
	for (uint32_t i = 0; i < families.size(); ++i) queueInfos[i].queueFamilyIndex = i;
	
	{ // assign queue family and index
		auto assignQueue = [&](vk::QueueFlags required, vk::QueueFlags excluded, Queue& output) {
			for (uint32_t i = 0; i < families.size(); ++i) {
				auto& family = families[i];
				auto& queueCount = queueInfos[i].queueCount;
				
				if ((family.queueFlags & required) != required) continue;
				if ((family.queueFlags & excluded) != vk::QueueFlags()) continue;
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

			throw std::runtime_error("error: no suitable queue found on selected device.");
		};

		auto assignPresentQueue = [&](Queue& output) {
			auto families = gpu.getQueueFamilyProperties();

			for (uint32_t i = 0; i < families.size(); ++i) {
				auto& family = families[i];
				
				if (!gpu.getSurfaceSupportKHR(i, *surface)) continue;
				if (!(family.queueFlags & vk::QueueFlagBits::eGraphics)) continue;

				output.family = i;
				output.index = 0;

				if (family.queueCount == 0) ++family.queueCount;
				
				return;
			}

			throw std::runtime_error("error: no suitable queue found on selected device.");
		};
		
		assignQueue(vk::QueueFlagBits::eGraphics, {}, graphics);
		assignQueue(vk::QueueFlagBits::eCompute, {}, compute);
		assignQueue(vk::QueueFlagBits::eTransfer, vk::QueueFlagBits::eGraphics, transfer);
		// requestQueue(vk::QueueFlagBits::eCompute, vk::QueueFlagBits::eGraphics, async);
		assignPresentQueue(present);

	}
	// assign priorities
	std::vector<float> priorities(std::ranges::fold_left(queueInfos, 0, [](uint32_t count, vk::DeviceQueueCreateInfo& family) {
		return count + family.queueCount;
	}));
	{
		float* priority = priorities.data();
		for (auto& family : queueInfos) {
			family.setPQueuePriorities(priority);
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
	
	info.setQueueCreateInfos(queueInfos);
	info.setPEnabledFeatures(&features);
	info.setPEnabledExtensionNames(deviceExtensions);
	auto device = gpu.createDevice(info);
	
	graphics.queue = device.getQueue(graphics.family, graphics.index);
	compute.queue = device.getQueue(compute.family, compute.index);
	transfer.queue = device.getQueue(transfer.family, transfer.index);
	present.queue = device.getQueue(present.family, present.index);
	
	return device;
}

Arawn::Engine::VmaAllocatorRAII Arawn::Engine::createAllocator() {
	VmaAllocatorCreateInfo info = {};
    info.vulkanApiVersion = VK_API_VERSION_1_3;
    info.physicalDevice = *gpu;
    info.device = *device;
    info.instance = *instance;
	
	
    VmaAllocator allocator;
    if (vmaCreateAllocator(&info, &allocator) != VK_SUCCESS) {
		throw std::runtime_error("failed to init vma allocator");
	}
	return VmaAllocatorRAII(allocator);
}


vk::raii::SwapchainKHR Arawn::Engine::createSwapchain() {
	vk::SurfaceCapabilitiesKHR capabilities = gpu.getSurfaceCapabilitiesKHR(surface);
	
	VkExtent2D extent;
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		extent.width  = std::clamp(capabilities.currentExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
		extent.height = std::clamp(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}
	
	uint32_t imageCount;
	imageCount = settings.display.buffering == BufferingMode::TRIPLE ? 3 : 2;
	imageCount = std::max(imageCount, capabilities.minImageCount);
	if (capabilities.maxImageCount > 0) {
		imageCount = std::min(imageCount, capabilities.maxImageCount);
	}

	vk::PresentModeKHR presentMode;
	auto presentModes = gpu.getSurfacePresentModesKHR(surface);
	if (settings.display.vsync == VsyncMode::ENABLED) {
		switch (settings.display.latency) {
		case LowLatencyMode::ENABLED:
		case LowLatencyMode::BALANCED:
			if (std::ranges::contains(presentModes, vk::PresentModeKHR::eMailbox)) {
				presentMode = vk::PresentModeKHR::eMailbox;
				break;
			}
		case LowLatencyMode::DISABLED:
			presentMode = vk::PresentModeKHR::eFifo;
			break;
		}
	} else {
		switch (settings.display.latency) {
		case LowLatencyMode::ENABLED:
			if (std::ranges::contains(presentModes, vk::PresentModeKHR::eImmediate)) {
				presentMode = vk::PresentModeKHR::eImmediate;
				break;
			}
		case LowLatencyMode::BALANCED:
		case LowLatencyMode::DISABLED:
			if (std::ranges::contains(presentModes, vk::PresentModeKHR::eFifoRelaxed)) {
				presentMode = vk::PresentModeKHR::eFifoRelaxed;
				break;
			}
			
			presentMode = vk::PresentModeKHR::eFifo;
			break;
		}
	}

	auto surfaceFormats = gpu.getSurfaceFormatsKHR(surface);
	surfaceFormat = surfaceFormats.front();
	for (const auto& candidate : surfaceFormats) { // if preferred format
		switch (candidate.format)
		{
			case(vk::Format::eR8G8B8A8Srgb): break;
			case(vk::Format::eB8G8R8A8Srgb): break;
			case(vk::Format::eR8G8B8A8Unorm): break;
			case(vk::Format::eB8G8R8A8Unorm): break;
			default: continue;
		}
		surfaceFormat = candidate;
	}	

	vk::SwapchainCreateInfoKHR info{ 
		{}, 
		surface, 
		imageCount, 
		surfaceFormat.format, 
		surfaceFormat.colorSpace, 
		extent,
		1, 
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive, 
		{},
		capabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		presentMode,
		VK_TRUE,
		nullptr
	};

	return vk::raii::SwapchainKHR(device, info);

}

void Arawn::Engine::VmaAllocatorDeleter::operator()(VmaAllocator_T* ptr) {
	vmaDestroyAllocator(ptr);
}
void Arawn::Engine::GLFWwindowDeleter::operator()(GLFWwindow* ptr) {
	glfwDestroyWindow(ptr);
}