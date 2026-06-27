#include <engine.h>

Arawn::Engine::Engine(const EngineSettings& engine, const DisplaySettings& display, const GraphicsSettings& graphics) 
 : settings(engine, display, graphics)
 , instance(createInstance())
 , gpu(selectGPU())
 , device(createDevice())
 , window(createWindow())
 , surface(createSurface())
 , swapchain(createSwapchain()) {




}

vk::raii::Instance Arawn::Engine::createInstance() {
	vk::ApplicationInfo app_info(settings.engine.appName, settings.engine.appVersion, settings.engine.engineName, settings.engine.engineVersion, VK_API_VERSION_1_4);
	vk::InstanceCreateInfo info({}, &app_info);
	
	std::vector<const char*> extensions = { };
	info.setPEnabledExtensionNames(extensions);

	std::vector<const char*> layers = { };
	info.setPEnabledLayerNames(layers);
	
	return vk::raii::Instance(context, info);
}

vk::raii::PhysicalDevice Arawn::Engine::selectGPU() {
	auto devices = instance.enumeratePhysicalDevices();

	if (devices.empty()) {
		throw std::runtime_error("no device found");
	}
	
	for (auto& device : devices) {
		if (strcmp(device.getProperties().deviceName, settings.engine.gpu)) {
			return device;
		}
	}
	
	for (auto& device : devices) {
		if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			return device;
		}
	}
	
	return devices[0];
}

vk::raii::Device Arawn::Engine::createDevice() {
	vk::DeviceCreateInfo info;
	auto families = gpu.getQueueFamilyProperties();
	
	std::vector<vk::DeviceQueueCreateInfo> queueInfos(families.size());
	for (uint32_t i = 0; i < families.size(); ++i) {
		queueInfos[i].queueFamilyIndex = i;
	}

	auto requestQueue = [&](vk::QueueFlags required, vk::QueueFlags excluded, Arawn::Engine::Queue& output) {
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

		throw std::runtime_error("failed to assign queue");
	};

	requestQueue(vk::QueueFlagBits::eGraphics, {}, graphics);
	requestQueue(vk::QueueFlagBits::eCompute, {}, compute);
	requestQueue(vk::QueueFlagBits::eTransfer, vk::QueueFlagBits::eGraphics, transfer);
	requestQueue(vk::QueueFlagBits::eCompute, vk::QueueFlagBits::eGraphics, async);
	
	queueInfos.erase(std::remove_if(queueInfos.begin(), queueInfos.end(), [](const auto& info) {
		return info.queueCount == 0;
	}), queueInfos.end());
	info.setQueueCreateInfos(queueInfos);
	
	vk::PhysicalDeviceFeatures features{ };
	info.setPEnabledFeatures(&features);

	std::vector<const char*> extensions = { };
	info.setPEnabledExtensionNames(extensions);

	auto device = gpu.createDevice(info);

	graphics.queue = device.getQueue(graphics.family, graphics.index);
	compute.queue = device.getQueue(compute.family, compute.index);
	transfer.queue = device.getQueue(transfer.family, transfer.index);
	async.queue = device.getQueue(async.family, async.index);
	
	return device;
}

Arawn::Engine::Window Arawn::Engine::createWindow() {
	glfwInit();
	GLFWwindow* window = glfwCreateWindow(800, 600, "", nullptr, nullptr);
	if (!window) throw std::runtime_error("failed to create window");
	return Window(window, glfwDestroyWindow);
}

vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance, const Arawn::Engine::Window& window) {
	VkSurfaceKHR surface;
	glfwCreateWindowSurface(*instance, window.get(), nullptr, &surface);
	return vk::raii::SurfaceKHR(instance, surface);
}

vk::raii::SwapchainKHR Arawn::Engine::createSwapchain() {
	vk::SurfaceCapabilitiesKHR capabilities = gpu.getSurfaceCapabilitiesKHR(surface);
	auto formats = gpu.getSurfaceFormatsKHR(surface);
	auto presentModes = gpu.getSurfacePresentModesKHR(surface);

	vk::SurfaceFormatKHR surfaceFormat = formats[0];
	for (const auto& format : formats) {
		if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {

		}
	}


}