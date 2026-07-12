#define ARAWN_INCLUDE_VULKAN
#define VMA_IMPLEMENTATION
#include <engine.h>
#include <vector>
#include <ranges>

constinit std::array instanceExtensions = std::to_array<const char*>({ 
#ifdef ARAWN_DEBUG
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
});
constinit std::array deviceExtensions = std::to_array<const char*>({
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
});
#ifdef ARAWN_DEBUG
constinit std::array debugLayers = std::to_array<const char*>({
	"VK_LAYER_KHRONOS_validation"
});
#endif

Arawn::Engine::Engine(const AppInfo& app, const DisplayInfo& display, QueueIndices&& queueIndices) 
 : state(app, display)
 , instance(createInstance())
#ifdef ARAWN_DEBUG
 , messenger(createMessenger())
#endif
 , window(createWindow())
 , surface(createSurface())
 , gpu(selectGPU())
 , device(createDevice(queueIndices)) 
 , queue({
	getQueue(queueIndices.graphics),
	getQueue(queueIndices.compute),
	getQueue(queueIndices.transfer),
	getQueue(queueIndices.present)
 })
 , allocator(createAllocator())
 , swapchain(createSwapchain())
 
{
	{ // init frame domain
		{ // allocate 
			uint32_t frameCount = display.buffering == BufferingMode::DOUBLE ? 2 : 3;
			Frame* frameData = static_cast<Frame*>(cache.allocate(sizeof(Frame) * frameCount, alignof(Frame)));
			domain.frame = { frameData, frameCount };

			ARAWN_LOG(DEBUG, std::format("render frames = {}", frameCount))
		}

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

			for (uint32_t i = 0; i < domain.frame.size(); ++i) {
				auto& ctx = domain.frame[i];
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

			for (uint32_t i = 0; i < domain.frame.size(); ++i) {
				auto& attachment = domain.frame[i].colorAttachment;
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

			for (uint32_t i = 0; i < domain.frame.size(); ++i) {
				auto& attachment = domain.frame[i].depthAttachment;
				VK_ASSERT(vmaCreateImage(allocator, &imgInfo, &allocInfo, &attachment.image, &attachment.memory, nullptr));

				viewInfo.image = attachment.image;
				VK_ASSERT(vkCreateImageView(device, &viewInfo, nullptr, &attachment.view));
			}
		}
	}

	{ // init swapchain domain
		{ // allocate
			uint32_t frameCount;
			vkGetSwapchainImagesKHR(device, swapchain, &frameCount, nullptr);
			Swap* swapData = static_cast<Swap*>(cache.allocate(sizeof(Swap) * frameCount, alignof(Swap)));
			domain.swap = { swapData, frameCount };

			ARAWN_LOG(DEBUG, std::format("swapchain frames = {}", frameCount))
		}

		{ // init sync primitves
			VkSemaphoreCreateInfo semaInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    			.pNext = nullptr,
    			.flags = 0,
			};
			
			for (uint32_t i = 0; i < domain.swap.size(); ++i) {
				VK_ASSERT(vkCreateSemaphore(device, &semaInfo, nullptr, &domain.swap[i].postprocessFinished));
			}
		}
		
		{ // init images
			uint32_t swapCount = static_cast<uint32_t>(domain.swap.size());
			std::vector<VkImage> images(domain.swap.size());
			vkGetSwapchainImagesKHR(device, swapchain, &swapCount, images.data());
			
			for (uint32_t i = 0; i < images.size(); ++i) {
				domain.swap[i].image = images[i];
			}
		}
	}

	{ // init forward pass
		{ // allocate
			uint32_t frameCount = static_cast<uint32_t>(domain.frame.size());
			Forward* frameData = static_cast<Forward*>(cache.allocate(sizeof(Forward) * frameCount, alignof(Forward)));
			pass.forward = { frameData, { frameCount } };

			ARAWN_LOG(DEBUG, std::format("forward frames = {}", frameCount))
		}
		
		{ // init command buffers
			VkCommandBufferAllocateInfo cmdInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = queue.graphics.pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = static_cast<uint32_t>(domain.frame.size()),
			};
	
			std::vector<VkCommandBuffer> cmds(cmdInfo.commandBufferCount);
			VK_ASSERT(vkAllocateCommandBuffers(device, &cmdInfo, cmds.data()));
	
			for (uint32_t cmdIndex = 0, frameIndex = 0; frameIndex < domain.frame.size(); ++frameIndex, ++cmdIndex) {
				auto& pass = this->pass.forward[frameIndex];
				pass.cmd = cmds[cmdIndex];
			}
		}

		{ // record
			for (uint32_t frameIndex = 0; frameIndex < domain.frame.size(); ++frameIndex) {
				auto& pass = this->pass.forward[frameIndex];
				pass.record(state, domain.shared, domain.frame[frameIndex]);
			}
		}
	}

	{ // init present pass
		{ // allocate
			uint32_t frameCount = static_cast<uint32_t>(domain.frame.size() * domain.swap.size());
			Present* frameData = static_cast<Present*>(cache.allocate(sizeof(Present) * frameCount, alignof(Present)));
			pass.present = { frameData, { domain.frame.size(), domain.swap.size() } };

			ARAWN_LOG(DEBUG, std::format("present frames = {}", frameCount))
		}
		
		{ // init cmd buffers
			VkCommandBufferAllocateInfo cmdInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = queue.present.pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = static_cast<uint32_t>(pass.present.size()),
			};
			
			std::vector<VkCommandBuffer> cmds(cmdInfo.commandBufferCount);

			VK_ASSERT(vkAllocateCommandBuffers(device, &cmdInfo, cmds.data()));

			for (uint32_t cmdIndex = 0, frameIndex = 0; frameIndex < domain.frame.size(); ++frameIndex) {
				for (uint32_t swapIndex = 0; swapIndex < domain.swap.size(); ++swapIndex, ++cmdIndex) {
					auto& pass = this->pass.present[frameIndex, swapIndex];
					pass.cmd = cmds[cmdIndex];
				}
			}
		}

		{ // record
			for (uint32_t frameIndex = 0; frameIndex < domain.frame.size(); ++frameIndex) {
				for (uint32_t swapIndex = 0; swapIndex < domain.swap.size(); ++swapIndex) {
					auto& pass = this->pass.present[frameIndex, swapIndex];
					pass.record(state, domain.shared, domain.frame[frameIndex], domain.swap[swapIndex]);
				}
			}
		}
	}
}

VkInstance Arawn::Engine::Engine::createInstance() const {
	if (!glfwInit()) throw std::runtime_error("error: failed to init glfw");
	
	uint32_t vulkanVersion = VK_API_VERSION_1_3;
		
	ARAWN_LOG(DEBUG, std::format("application version={}.{}.{}", state.version.major, state.version.minor, state.version.patch));
	ARAWN_LOG(DEBUG, std::format("vulkan api version={}.{}.{}", VK_API_VERSION_MAJOR(vulkanVersion), VK_API_VERSION_MINOR(vulkanVersion), VK_API_VERSION_PATCH(vulkanVersion)));
	
	std::vector<const char*> extensions;
	{
		uint32_t glfwExtCount;
		const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
		extensions.append_range(std::span{ glfwExts, glfwExts + glfwExtCount });
		extensions.append_range(instanceExtensions);
	}
	
	std::ranges::sort(extensions, [](const char* lhs, const char* rhs)->bool {
		return std::strcmp(lhs, rhs);
	});
	
	extensions.erase(std::ranges::unique(extensions, [](const char* lhs, const char* rhs)->bool {
		return std::strcmp(lhs, rhs) == 0;
	}).end(), extensions.end());
	
	ARAWN_LOG(DEBUG, std::format("instance extensions={}", extensions));
			
	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, 
		.pNext = nullptr, 
		.pApplicationName = state.title,
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
#ifdef ARAWN_DEBUG
		.enabledLayerCount = static_cast<uint32_t>(debugLayers.size()),
		.ppEnabledLayerNames = debugLayers.data(),
#endif
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
	};
	
	VkInstance instance;
	VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &instance))
	
	return instance;
}


#ifdef ARAWN_DEBUG
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
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: ARAWN_LOG(VERBOSE, info->pMessage) break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: ARAWN_LOG(VERBOSE, info->pMessage) break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: ARAWN_LOG(WARNING, info->pMessage) break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: throw std::runtime_error(ARAWN_LOG_MESSAGE(ERROR, info->pMessage));
		default: break;
	}
	
	return VK_FALSE;
}
PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessengerEXT;

VkDebugUtilsMessengerEXT Arawn::Engine::createMessenger() const {
	if (createDebugUtilsMessengerEXT == nullptr) {
		createDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		destroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	}

	VkDebugUtilsMessengerCreateInfoEXT info{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = nullptr,
		.flags = 0, 
		.messageSeverity = 0,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = vulkanDebugger,
		.pUserData = nullptr
	};

#ifdef ARAWN_LOG_VERBOSE
		info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
#endif

#ifdef ARAWN_LOG_DEBUG
		info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
#endif

#ifdef ARAWN_LOG_WARNING
		info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
#endif

#ifdef ARAWN_LOG_ERROR
		info.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
#endif

	VkDebugUtilsMessengerEXT messenger;
	VK_ASSERT(createDebugUtilsMessengerEXT(instance, &info, nullptr, &messenger));
	return messenger;
}
#endif

GLFWwindow* Arawn::Engine::createWindow() const {
	// select video mode
	auto selectVideoMode = [&](GLFWmonitor* monitor, int targetWidth, int targetHeight, int targetRefresh)->const GLFWvidmode& {
		int count;
		const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);
		return *std::ranges::min_element(std::span{ modes, modes + count }, {}, [&](const GLFWvidmode& mode)->uint32_t { 
			uint32_t width = std::abs(mode.width - targetWidth); 
			uint32_t height = std::abs(mode.height - targetHeight); 
			uint32_t refreshRateDiff = std::abs(mode.refreshRate - targetRefresh); 

			return (width + height) * 10000 + refreshRateDiff;
		});
	};

	if (window != nullptr) {
		auto resetVideoMode = [&](GLFWmonitor* monitor) {
			int count;
			const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);
			const GLFWvidmode& mode = modes[count - 1];

			glfwSetWindowMonitor(
				window, 
				monitor, 
				0, 0, 
				mode.width, mode.height,
				mode.refreshRate
			);
		};

		auto selectMonitor = [&](GLFWwindow* window)->GLFWmonitor* {
			int count;
			GLFWmonitor** monitorData = glfwGetMonitors(&count);
			std::span monitors{ monitorData, monitorData + count };

			if (state.monitor != nullptr) {
				auto it = std::ranges::find(monitors, std::string_view{ state.monitor }, [&](GLFWmonitor* monitor)->std::string_view { 
					return { glfwGetMonitorName(monitor) };
				});

				if (it != monitors.end()) {
					return *it;
				}
			}
			
			struct { int x, y, width, height; } win;
			glfwGetWindowPos(window, &win.x, &win.y);
			glfwGetWindowSize(window, &win.width, &win.height);

			return *std::ranges::max_element(monitors, {}, [&](GLFWmonitor* monitor)->uint32_t {
				struct { int x, y, width, height; } mon;
				glfwGetMonitorPos(monitor, &mon.x, &mon.y);
				
				const GLFWvidmode& mode = *glfwGetVideoMode(monitor);
				mon.width = mode.width;
				mon.height = mode.height;

				int x1 = std::max(win.x, mon.x);
				int y1 = std::max(win.y, mon.y);
				int x2 = std::min(win.x + win.width, mon.x + mode.width);
				int y2 = std::min(win.y + win.height, mon.y + mode.height);

				return std::max(0, x2 - x1) * std::max(0, y2 - y1);
			});
		};
		
		switch(state.display) {
		case DisplayMode::WINDOWED: {
			GLFWmonitor* monitor = glfwGetWindowMonitor(window); 
			if (monitor != nullptr) {
				resetVideoMode(monitor);
			}
			
			glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
			glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_FALSE);
			
			glfwSetWindowMonitor(
				window, 
				nullptr, 
				100, 100, 
				state.resolution.x, state.resolution.y,
				state.refreshRate
			);
			break;
		}
		case DisplayMode::FULLSCREEN: {
			GLFWmonitor* monitor = glfwGetWindowMonitor(window); 
			if (monitor != nullptr) {
				resetVideoMode(monitor);
			} else {
				monitor = selectMonitor(window);
			}
			
			glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
			glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
			
			struct { int x, y; } mon;
			glfwGetMonitorPos(monitor, &mon.x, &mon.y);
			const GLFWvidmode& mode = *glfwGetVideoMode(monitor);
			
			glfwSetWindowMonitor(
				window,
				nullptr,
				mon.x, mon.y,
				mode.width, mode.height,
				state.refreshRate
			);

			break;
		}
		case Arawn::DisplayMode::EXCLUSIVE: {
			GLFWmonitor* monitor = glfwGetWindowMonitor(window); 
			if (monitor == nullptr) {
				monitor = selectMonitor(window);	
			}

			const GLFWvidmode& mode = selectVideoMode(monitor, state.resolution.x, state.resolution.y, state.refreshRate);

			glfwSetWindowMonitor(
				window,
				monitor,
				0, 0,
				mode.width, mode.height,
				mode.refreshRate
			);

			break;
		}
		}
		
		return window;
	} else {
		auto selectMonitor = [&]{
			int count;
			GLFWmonitor** monitorData = glfwGetMonitors(&count);
			std::span monitors{ monitorData, monitorData + count };

			if (state.monitor != nullptr) {
				auto it = std::ranges::find(monitors, std::string_view{ state.monitor }, [&](GLFWmonitor* monitor) { 
					return std::string_view{ glfwGetMonitorName(monitor) };
				});

				if (it != monitors.end()) {
					return *it;
				}
			}

			return glfwGetPrimaryMonitor();
		};

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		glfwWindowHint(GLFW_REFRESH_RATE, state.refreshRate);
		
		GLFWwindow* window;
		switch(state.display) {
		case DisplayMode::WINDOWED: {
			glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
			glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
			
			window = glfwCreateWindow(
				state.resolution.x,
				state.resolution.y,
				state.title,
				nullptr,
				nullptr
			);
			break;
		}
		case DisplayMode::FULLSCREEN: {
			GLFWmonitor* monitor = selectMonitor();
			const GLFWvidmode& mode = *glfwGetVideoMode(monitor);
			
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
	
			window = glfwCreateWindow(
				mode.width, 
				mode.height,
				state.title,
				nullptr,
				nullptr
			);
			break;
		}
		case DisplayMode::EXCLUSIVE: {
			GLFWmonitor* monitor = selectMonitor();
			const GLFWvidmode& mode = selectVideoMode(monitor, state.resolution.x, state.resolution.y, state.refreshRate);
			
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
			glfwWindowHint(GLFW_REFRESH_RATE, mode.refreshRate);

			window = glfwCreateWindow(
				mode.width,
				mode.height,
				state.title,
				monitor,
				nullptr
			);
			break;
		}	
		}
	
		if (!window) [[unlikely]] {
			const char* message;
			glfwGetError(&message);
	
			std::printf("glfw error: %s\n", message);
			throw std::runtime_error(ARAWN_LOG_MESSAGE(ERROR, "failed to initialize window"));
		}
	
		return window;
	}
}

VkSurfaceKHR Arawn::Engine::createSurface() const {
	VkSurfaceKHR surface;
	VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
	return surface;
}

VkPhysicalDevice Arawn::Engine::selectGPU() const {
	VkPhysicalDevice gpu;
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

		// VkPhysicalDeviceFeatures features;
		// vkGetPhysicalDeviceFeatures(gpu, &features);

		return false;
	};

	auto scoreGPU = [&](VkPhysicalDevice gpu) -> uint32_t {
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
		
		return score;
	};
	
	uint32_t gpuCount;
	VK_ASSERT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
	std::vector<VkPhysicalDevice> gpus(gpuCount);
	VK_ASSERT(vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data()));

	if (state.gpu == nullptr || [&]{ 
		auto it = std::ranges::find_if(gpus, [&](const auto& gpu) { 
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(gpu, &properties);
			return strcmp(properties.deviceName, state.gpu);
		});
		
		if (it == gpus.end()) {
			ARAWN_LOG(WARNING, std::format("gpu \"{}\" not found.", state.gpu));
			return true;
		}

		if (unsupportedGPU(*it)) {
			ARAWN_LOG(WARNING, std::format("gpu \"{}\" not supported.", state.gpu));
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
	
	ARAWN_LOG(DEBUG, std::format("gpu=\"{}\"", [&]->std::string { 
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(gpu, &properties);
		return properties.deviceName;
	}()));

	return gpu;
}

VkDevice Arawn::Engine::createDevice(QueueIndices& queues) const {
	uint32_t familyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families.data());

	std::vector<VkDeviceQueueCreateInfo> queueInfos(families.size(), { 
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0
	});
	for (uint32_t i = 0; i < families.size(); ++i) queueInfos[i].queueFamilyIndex = i;

	{ // assign queue family and index
		auto assignQueue = [&](VkQueueFlags required, VkQueueFlags excluded, QueueIndex& output) {
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
	
		auto assignPresentQueue = [&](QueueIndex& output) {
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
		
		assignQueue(VK_QUEUE_GRAPHICS_BIT, 0, queues.graphics);
		assignQueue(VK_QUEUE_COMPUTE_BIT, 0, queues.compute);
		assignQueue(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT, queues.transfer);
		assignPresentQueue(queues.present);
	}

	// assign priorities
	uint32_t queueCount = std::ranges::fold_left(queueInfos, 0, [](uint32_t count, const VkDeviceQueueCreateInfo& family) {
		return count + family.queueCount;
	});
	std::vector<float> priorities(queueCount);
	{
		float* priority = priorities.data();
		for (auto& family : queueInfos) {
			family.pQueuePriorities = priority;
			priority += family.queueCount;
		}
	
		auto assignPriority = [&](const QueueIndex& input, float priority) {
			float& queuePriority = ((queueInfos[input.family].pQueuePriorities - priorities.data()) + priorities.data())[input.index];
			queuePriority = std::max<float>(priority, queuePriority);
		};
	
		assignPriority(queues.graphics, 0.8);
		assignPriority(queues.compute, 0.6);
		assignPriority(queues.transfer, 0.4);
		assignPriority(queues.present, 1.0);
	}

	{ // create device
		ARAWN_LOG(DEBUG, std::format("device.graphicsQueue = {{ family = {}, index = {}, priority = {} }}", queues.graphics.family, queues.graphics.index, queueInfos[queues.graphics.family].pQueuePriorities[queues.graphics.index]));
		ARAWN_LOG(DEBUG, std::format("device.computeQueue = {{ family = {}, index = {}, priority = {} }}", queues.compute.family, queues.compute.index, queueInfos[queues.compute.family].pQueuePriorities[queues.compute.index]));
		ARAWN_LOG(DEBUG, std::format("device.transferQueue = {{ family = {}, index = {}, priority = {} }}", queues.transfer.family, queues.transfer.index, queueInfos[queues.transfer.family].pQueuePriorities[queues.transfer.index]));
		ARAWN_LOG(DEBUG, std::format("device.presentQueue = {{ family = {}, index = {}, priority = {} }}", queues.present.family, queues.present.index, queueInfos[queues.present.family].pQueuePriorities[queues.present.index]));
	
		// reduce queues -> loses family mapping
		std::erase_if(queueInfos, [](const auto& queueInfo) {
			return queueInfo.queueCount == 0;
		});
	
		ARAWN_LOG(DEBUG, std::format("device extensions={}", deviceExtensions));
	
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
		VkDevice device;
		VK_ASSERT(vkCreateDevice(gpu, &info, nullptr, &device))
		return device;
	}
}

Arawn::Engine::Queue Arawn::Engine::getQueue(const QueueIndex& indices) const {
	VkQueue queue;
	VkCommandPool pool;
	vkGetDeviceQueue(device, indices.family, indices.index, &queue);

	VkCommandPoolCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = indices.family,
	};
	VK_ASSERT(vkCreateCommandPool(device, &createInfo, nullptr, &pool));

	return { indices, queue, pool };
}
		
VmaAllocator Arawn::Engine::createAllocator() const {
	VmaAllocatorCreateInfo createInfo = {
		.flags = 0,
		.physicalDevice = gpu,
		.device = device,
		.instance = instance,
		.vulkanApiVersion = VK_API_VERSION_1_3,
	};
	VmaAllocator allocator;
	VK_ASSERT(vmaCreateAllocator(&createInfo, &allocator));
	return allocator;
}

VkSwapchainKHR Arawn::Engine::createSwapchain() const {
	VkSurfaceFormatKHR format = [&]->VkSurfaceFormatKHR  {

		uint32_t formatCount;
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr));
		std::vector<VkSurfaceFormatKHR> supported(formatCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, supported.data()));
		
		for (const auto& candidate : supported) { // if preferred format
			switch (candidate.format)
			{
				case(VK_FORMAT_R8G8B8A8_SRGB): break;
				case(VK_FORMAT_B8G8R8A8_SRGB): break;
				case(VK_FORMAT_R8G8B8A8_UNORM): break;
				case(VK_FORMAT_B8G8R8A8_UNORM): break;
				default: continue;
			}
		
			return candidate;
		}

		return supported.front();
	}();

	VkSurfaceCapabilitiesKHR capabilities;
	VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capabilities));
	
	VkSwapchainCreateInfoKHR createInfo{ 
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = surface,
		.imageFormat = format.format,
		.imageColorSpace = format.colorSpace,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = nullptr,
	};
	
	ARAWN_ASSERT(((capabilities.supportedUsageFlags & createInfo.imageUsage) == createInfo.imageUsage), "swapchain image usage not supported");
	
	std::array<uint32_t, 2> families = { queue.graphics.family, queue.present.family };
	if (queue.graphics.family == queue.present.family) {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.pQueueFamilyIndices = families.data();
		createInfo.queueFamilyIndexCount = 2;
	}
	
	if (capabilities.currentExtent.width != UINT32_MAX) {
		createInfo.imageExtent = capabilities.currentExtent;
	} else {
		createInfo.imageExtent = {
			std::clamp(state.resolution.x, capabilities.minImageExtent.width,  capabilities.maxImageExtent.width),
			std::clamp(state.resolution.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}
	
	createInfo.minImageCount = std::max(state.buffering == BufferingMode::TRIPLE ? 3u : 2u, capabilities.minImageCount);
	if (capabilities.maxImageCount > 0) {
		createInfo.minImageCount = std::min(createInfo.minImageCount, capabilities.maxImageCount);
	}
	
	uint32_t presentModeCount;
	VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, nullptr));
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, presentModes.data()));
	
	if (state.vsync == VsyncMode::ENABLED) {
		switch (state.latency) {
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
		switch (state.latency) {
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
	
	ARAWN_LOG(DEBUG, std::format("swapchain extent = {{ {}, {} }}", createInfo.imageExtent.width, createInfo.imageExtent.height))
	ARAWN_LOG(DEBUG, std::format("swapchain present mode={}", string_VkPresentModeKHR(createInfo.presentMode)));
	ARAWN_LOG(DEBUG, std::format("swapchain image format={}", string_VkFormat(createInfo.imageFormat)));
	ARAWN_LOG(DEBUG, std::format("swapchain image color space={}", string_VkColorSpaceKHR(createInfo.imageColorSpace)));
	
	VkSwapchainKHR swapchain;
	VK_ASSERT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain));
	return swapchain;

}

Arawn::Engine::~Engine() noexcept {
	if (instance != nullptr) {
		vkDeviceWaitIdle(device);

		for (uint32_t i = 0; i < pass.forward.size(); ++i) {
			auto& pass = this->pass.forward.data_handle()[i];
			vkFreeCommandBuffers(device, queue.graphics.pool, 1, &pass.cmd);
		}
		
		for (uint32_t i = 0; i < pass.present.size(); ++i) {
			auto& pass = this->pass.present.data_handle()[i];
			vkFreeCommandBuffers(device, queue.present.pool, 1, &pass.cmd);
		}
		
		for (uint32_t i = 0; i < domain.frame.size(); ++i) {
			auto& ctx = domain.frame[i];
			vkDestroySemaphore(device, ctx.imageAvailable, nullptr);
			vkDestroySemaphore(device, ctx.forwardFinished, nullptr);
			vkDestroyFence(device, ctx.inFlight, nullptr);

			vkDestroyImageView(device, ctx.colorAttachment.view, nullptr);
			vmaDestroyImage(allocator, ctx.colorAttachment.image, ctx.colorAttachment.memory);

			vkDestroyImageView(device, ctx.depthAttachment.view, nullptr);
			vmaDestroyImage(allocator, ctx.depthAttachment.image, ctx.depthAttachment.memory);
		}
		
		for (uint32_t i = 0; i < domain.swap.size(); ++i) {
			auto& ctx = domain.swap[i];
			vkDestroySemaphore(device, ctx.postprocessFinished, nullptr);
		}
		cache.release();

		vkDestroySwapchainKHR(device, swapchain, nullptr);
		vmaDestroyAllocator(allocator);
		vkDestroyCommandPool(device, queue.graphics.pool, nullptr);
		vkDestroyCommandPool(device, queue.compute.pool, nullptr);
		vkDestroyCommandPool(device, queue.transfer.pool, nullptr);
		vkDestroyCommandPool(device, queue.present.pool, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		glfwDestroyWindow(window);
#ifdef ARAWN_DEBUG
		destroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
#endif
		vkDestroyInstance(instance, nullptr);
	}
}

bool Arawn::Engine::closed() const {
	glfwPollEvents();
	return glfwWindowShouldClose(window);
}

void Arawn::Engine::render() {
	Frame& frame = domain.frame[domain.frame.index];

	VK_ASSERT(vkWaitForFences(device, 1, &frame.inFlight, VK_TRUE, 1000000000));
	
	VK_ASSERT(vkResetFences(device, 1, &frame.inFlight));

	{ // forward pass
		Forward& forwardPass = pass.forward[domain.frame.index];

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
		VkResult res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, frame.imageAvailable, nullptr, &domain.swap.index);
		if (res == VK_ERROR_OUT_OF_DATE_KHR) [[unlikely]] {
			// recreate swapchain
		} else if (res == VK_SUBOPTIMAL_KHR) [[unlikely]] {
			// recreate swapchain???
		} else VK_ASSERT(res);
	}
	Swap& swap = domain.swap[domain.swap.index];
	
	{ // post process pass
		Present& presentPass = pass.present[domain.frame.index, domain.swap.index];
		auto* x = &pass.present[domain.frame.index, domain.swap.index];

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
			.pImageIndices = &domain.swap.index,
			.pResults = nullptr,
		};

		VkResult res = vkQueuePresentKHR(queue.present.queue, &submitInfo);
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR /* or frame buffer resized */) {
			// recreate swapchain
		} else VK_ASSERT(res)
	}
	
	// increment contexts
	domain.frame.index = (domain.frame.index + 1) % domain.frame.size();
}

void Arawn::Engine::Forward::record(const State& state, const Shared& shared, const Frame& frame) {
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

void Arawn::Engine::Present::record(const State& state, const Shared& shared, const Frame& frame, const Swap& swap) {
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