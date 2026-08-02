#define ARAWN_INCLUDE_VULKAN
#include <engine/device.h>
#include <vector>
#include <algorithm>
#include <ranges>
#include <cstring>

using namespace arawn;

constinit std::array deviceExtensions = std::to_array<const char*>({
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
});

void engine::Device::create(const Core& core, const Window& window, const Settings& info) {
	
	auto getQueue = [&](Queue& queue) {
		vkGetDeviceQueue(device, queue.family, queue.index, &queue.queue);

		VkCommandPoolCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueFamilyIndex = queue.family,
		};
		VK_ASSERT(vkCreateCommandPool(device, &createInfo, nullptr, &queue.pool));
	};

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
				vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, window.surface, &supported);
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
		VK_ASSERT(vkEnumeratePhysicalDevices(core.instance, &gpuCount, nullptr));
		std::vector<VkPhysicalDevice> gpus(gpuCount);
		VK_ASSERT(vkEnumeratePhysicalDevices(core.instance, &gpuCount, gpus.data()));
	
		if (info.gpu == nullptr || [&]{ 
			auto it = std::ranges::find_if(gpus, [&](const auto& gpu) { 
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpu, &properties);
				return strcmp(properties.deviceName, info.gpu);
			});
			
			if (it == gpus.end()) {
				ARAWN_LOG(WARNING, std::format("gpu \"{}\" not found.", info.gpu));
				return true;
			}
		
			if (unsupportedGPU(*it)) {
				ARAWN_LOG(WARNING, std::format("gpu \"{}\" not supported.", info.gpu));
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
		
		ARAWN_LOG(VERBOSE, std::format("gpu=\"{}\"", [&]->std::string { 
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(gpu, &properties);
			return properties.deviceName;
		}()));
	}
	
	
	std::vector<VkQueueFamilyProperties> families;
	std::vector<VkDeviceQueueCreateInfo> queueInfos;
	{ // reserve data
		uint32_t familyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
		families = std::vector<VkQueueFamilyProperties>(familyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, families.data());
	
		queueInfos = std::vector<VkDeviceQueueCreateInfo>(families.size(), { 
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		});
		for (uint32_t i = 0; i < families.size(); ++i) queueInfos[i].queueFamilyIndex = i;

	}

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
				VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, window.surface, &supported));
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
	
		auto assignPriority = [&](const Queue& input, float priority) {
			float& queuePriority = ((queueInfos[input.family].pQueuePriorities - priorities.data()) + priorities.data())[input.index];
			queuePriority = std::max<float>(priority, queuePriority);
		};
	
		assignPriority(queue.graphics, 0.8);
		assignPriority(queue.compute, 0.6);
		assignPriority(queue.transfer, 0.4);
		assignPriority(queue.present, 1.0);

		ARAWN_LOG(VERBOSE, std::format("device.graphicsQueue = {{ family = {}, index = {}, priority = {} }}", queue.graphics.family, queue.graphics.index, queueInfos[queue.graphics.family].pQueuePriorities[queue.graphics.index]));
		ARAWN_LOG(VERBOSE, std::format("device.computeQueue = {{ family = {}, index = {}, priority = {} }}", queue.compute.family, queue.compute.index, queueInfos[queue.compute.family].pQueuePriorities[queue.compute.index]));
		ARAWN_LOG(VERBOSE, std::format("device.transferQueue = {{ family = {}, index = {}, priority = {} }}", queue.transfer.family, queue.transfer.index, queueInfos[queue.transfer.family].pQueuePriorities[queue.transfer.index]));
		ARAWN_LOG(VERBOSE, std::format("device.presentQueue = {{ family = {}, index = {}, priority = {} }}", queue.present.family, queue.present.index, queueInfos[queue.present.family].pQueuePriorities[queue.present.index]));
	}

	{ // create device
		// reduce queues -> loses family mapping
		std::erase_if(queueInfos, [](const auto& queueInfo) {
			return queueInfo.queueCount == 0;
		});
	
		ARAWN_LOG(VERBOSE, std::format("device extensions={}", deviceExtensions));
	
		VkPhysicalDeviceFeatures features{ };
		VkDeviceCreateInfo deviceInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
			.pQueueCreateInfos = queueInfos.data(),
			.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = &features,
		};
		VK_ASSERT(vkCreateDevice(gpu, &deviceInfo, nullptr, &device));
	}

	{ // get queues
		getQueue(queue.graphics);
		getQueue(queue.compute);
		getQueue(queue.transfer);
		getQueue(queue.present);
	}

	{ // create allocator
		VmaAllocatorCreateInfo allocatorInfo = {
			.flags = 0,
			.physicalDevice = gpu,
			.device = device,
			.instance = core.instance,
			.vulkanApiVersion = ARAWN_VULKAN_VERSION,
		};
		VK_ASSERT(vmaCreateAllocator(&allocatorInfo, &allocator));
	}

	{ // create descriptor pool - dont know how many are needed
		auto sizes = std::to_array({
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 64 },
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64 },
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, 64 },
		});
		VkDescriptorPoolCreateInfo poolInfo {
    		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    		.pNext = nullptr,
    		.flags = 0,
    		.maxSets = 128,
    		.poolSizeCount = static_cast<uint32_t>(sizes.size()),
    		.pPoolSizes = sizes.data(),
		};

		VK_ASSERT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptor.pool));
	}
}

void engine::Device::recreate(const Core& core, const Window& window, const Settings& info) {
	// destroy all command buffers
	throw std::logic_error("not implemented yet");


	vkDestroyCommandPool(device, queue.graphics.pool, nullptr);
	vkDestroyCommandPool(device, queue.compute.pool, nullptr);
	vkDestroyCommandPool(device, queue.transfer.pool, nullptr);
	vkDestroyCommandPool(device, queue.present.pool, nullptr);

	VkDevice oldDevice = device;
	VmaAllocator oldAllocator = allocator;

	create(core, window, info);

	// TODO:
	// destroy dependencies
	// ? possibly transfer dependencies
	// create dependencies

	vmaDestroyAllocator(allocator);
	vkDestroyDevice(device, nullptr);
}

void engine::Device::destroy(const Core& core) {
	vmaDestroyAllocator(allocator);

	vkDestroyCommandPool(device, queue.graphics.pool, nullptr);
	vkDestroyCommandPool(device, queue.compute.pool, nullptr);
	vkDestroyCommandPool(device, queue.transfer.pool, nullptr);
	vkDestroyCommandPool(device, queue.present.pool, nullptr);

	vkDestroyDevice(device, nullptr);
}