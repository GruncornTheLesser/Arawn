
#define ARAWN_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#ifndef VULKAN_VERSION
#define VULKAN_VERSION VK_API_VERSION_1_3
#endif 

#include <render/core/engine.h>
#include <vector>
#include <algorithm>
#include <cstring>

Arawn::Engine::Engine(const Info& info) {
    
    { // init glfw
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        GLFW_ASSERT(glfwInit() == GLFW_TRUE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    { // print vulkan version
        uint32_t major = VK_VERSION_MAJOR(VULKAN_VERSION);
        uint32_t minor = VK_VERSION_MINOR(VULKAN_VERSION);
        uint32_t patch = VK_VERSION_PATCH(VULKAN_VERSION);
        LOG("vk version: " << major << "." << minor << "." << patch);
    }
    
    { // init instance
        #ifdef ARAWN_DEBUG
        std::vector<const char*> instanceLayers = { 
            "VK_LAYER_KHRONOS_validation"
        };
        #endif
        
        std::vector<const char*> instanceExtensions = {
            VK_KHR_SURFACE_EXTENSION_NAME
        };
        
        #ifdef ARAWN_DEBUG
        { // remove unsupported layers support
            uint32_t count;
            VK_ASSERT(vkEnumerateInstanceLayerProperties(&count, nullptr));
            
            std::vector<VkLayerProperties> available(count);
            VK_ASSERT(vkEnumerateInstanceLayerProperties(&count, available.data()));
    
            // remove layer if not supported 
            instanceLayers.erase(std::remove_if(instanceLayers.begin(), instanceLayers.end(), [&](const char* layer) {
                auto it = std::find_if(available.begin(), available.end(), [&](const VkLayerProperties& properties) { 
                    return strcmp(layer, properties.layerName) == 0; 
                });
    
                if (it == available.end()) {
                    LOG("unsupported instance debug layer: " << layer);
                    return false;
                }
                return true;
            }), instanceLayers.end());
        }
        #endif
        
        uint32_t dynamicOffsetCount;
        const uint32_t *pDynamicOffset
        
        { // add glfw vulkan extensions 
            uint32_t count;
            const char** exts = glfwGetRequiredInstanceExtensions(&count);
            std::copy(exts, exts + count, std::back_insert_iterator(instanceExtensions));
            
            // remove duplicate extensions
            instanceExtensions.erase(std::unique(instanceExtensions.begin(), instanceExtensions.end()), instanceExtensions.end());
        }

        { // check instance extension support
            uint32_t count;
            VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
    
            std::vector<VkExtensionProperties> available(count);
            VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data()));
            
            for (const char* extension : instanceExtensions) {
                auto it = std::find_if(available.begin(), available.end(), 
                    [&](const VkExtensionProperties& p) { return strcmp(extension, p.extensionName) == 0; });
                
                if (it == available.end())
                    throw std::runtime_error("instance " + std::string(extension) + " extension not supported");
            }
        }

        {
            VkApplicationInfo app{
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, 
                .pNext = nullptr, 
                .pApplicationName = info.app_name, 
                .applicationVersion = VK_MAKE_VERSION(1, 0, 0), 
                .pEngineName = "Arawn-Engine", 
                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                .apiVersion = VULKAN_VERSION
            };
    
            VkInstanceCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 
                    .pNext = nullptr,
                    .flags = 0, 
                    .pApplicationInfo = &app,
                #ifdef ARAWN_DEBUG
                    .enabledLayerCount = static_cast<uint32_t>(instanceLayers.size()),
                    .ppEnabledLayerNames = instanceLayers.data(),
                #else
                    .enabledLayerCount = 0,
                    .ppEnabledLayerNames = nullptr,
                #endif
                    .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()), 
                    .ppEnabledExtensionNames = instanceExtensions.data()
            };
        
            VK_ASSERT(vkCreateInstance(&info, nullptr, &instance));
        }
    }

    { // init device
        std::vector<const char*> deviceExtensions = { 
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        
        { // select gpu
            uint32_t count = 0;
            VK_ASSERT(vkEnumeratePhysicalDevices(instance, &count, nullptr));
    
            if (count == 0)
                throw std::runtime_error("failed to find physical device with vulkan support");
    
            std::vector<VkPhysicalDevice> devices(count);
            vkEnumeratePhysicalDevices(instance, &count, devices.data());
            
            uint32_t index = 0; // defaults to first
            if (info.device_name != nullptr) {
                for (uint32_t i = 0; i < count; ++i) {
                    VkPhysicalDeviceProperties properties;
                    vkGetPhysicalDeviceProperties(devices[i], &properties);
                    
                    if (strcmp(info.device_name, properties.deviceName) == 0) {
                        index = i;
                        break;
                    }
                }
            }
            gpu = devices[index];
    
            #ifdef ARAWN_DEBUG
            { // log gpu selection
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(gpu, &properties);
        
                LOG("gpu: " << properties.deviceName);
            }
            #endif
        }

        { // check device extension support
            uint32_t count;
            vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr);
    
            std::vector<VkExtensionProperties> available(count);
            vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, available.data());
            
            for (const char* extension : deviceExtensions) {
                auto it = std::find_if(available.begin(), available.end(), [&](const VkExtensionProperties& p) { 
                    return strcmp(extension, p.extensionName) == 0; 
                });
                
                if (it == available.end())
                    throw std::runtime_error("device extension not supported");
            }

            { // check device feature support
                VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{ 
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES, 
                    .pNext = nullptr
                };
                VkPhysicalDeviceFeatures2 supported{
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, 
                    .pNext = &indexingFeatures
                };

                vkGetPhysicalDeviceFeatures2(gpu, &supported);

                if (!supported.features.samplerAnisotropy) 
                    throw std::runtime_error("gpu does not support sampler anisotropy feature");

                if (!indexingFeatures.descriptorBindingPartiallyBound || !indexingFeatures.runtimeDescriptorArray)
                    throw std::runtime_error("gpu does not support bindless rendering");
            }
        }

        std::vector<VkDeviceQueueCreateInfo> queueInfo;
        std::vector<float> queuePriorities;
        { // get queues
            uint32_t familyCount;
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
            
            std::vector<VkQueueFamilyProperties> queueFamilies(familyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, queueFamilies.data());
            
            queueInfo.resize(familyCount, { 
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = UINT32_MAX,
                .queueCount = 0,
                .pQueuePriorities = nullptr,
            });

            for (uint32_t i = 0; i < familyCount; ++i) {
                queueInfo[i].queueFamilyIndex = i;
            }
    
            auto findFamily = [&](uint32_t include, uint32_t exclude) {
                for (uint32_t i = 0; i < familyCount; ++i) {
                    // if max queues reached for this family
                    if (queueInfo[i].queueCount >= queueFamilies[i].queueCount) {
                        continue;
                    }
                    
                    // if include flags not found
                    if (include != (queueFamilies[i].queueFlags & include)) {
                        continue;
                    }
                    
                    // if exclude flag found 
                    if (0 != (queueFamilies[i].queueFlags & exclude)) {
                        continue;
                    }
    
                    return i;
                }
                return UINT32_MAX;
            };

            // find graphics queue
            {
                // must implement graphics, prefer if implements compute
                if (queue[GRAPHICS].family = findFamily(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0); queue[GRAPHICS].family != UINT32_MAX) {
                    queue[GRAPHICS].index = queueInfo[queue[GRAPHICS].family].queueCount++; // supported by almost all devices
                }
                else if (queue[GRAPHICS].family = findFamily(VK_QUEUE_GRAPHICS_BIT, 0); queue[GRAPHICS].family != UINT32_MAX) {
                    queue[GRAPHICS].index = queueInfo[queue[GRAPHICS].family].queueCount++;
                }
                else {
                    throw std::runtime_error("failed to find family for GRAPHICS queue");
                }
            }
    
            // find compute queue
            {
                // prefer separate queue, shared family to reduce inter-family synchronizations
                if (queueFamilies[queue[GRAPHICS].family].queueFlags & VK_QUEUE_COMPUTE_BIT) [[likely]] {
                    queue[COMPUTE].family = queue[GRAPHICS].family; 
                    
                    // graphics families usually support compute, prefer seperate queue for better throughput

                    if (queueFamilies[queue[GRAPHICS].family].queueCount > queueInfo[queue[GRAPHICS].family].queueCount) {
                        queue[COMPUTE].index = queueInfo[queue[COMPUTE].family].queueCount++;
                    }
                    else {
                        queue[COMPUTE].index = queue[GRAPHICS].index;
                    }
                }
                else if (queue[COMPUTE].family = findFamily(VK_QUEUE_COMPUTE_BIT, 0); queue[COMPUTE].family != UINT32_MAX) {
                    queue[COMPUTE].index = queueInfo[queue[COMPUTE].family].queueCount++;
                }
                else {
                    throw std::runtime_error("failed to find family for COMPUTE queue");
                }
            }
            
            // find transfer family
            {
                // prefer exclusive queue in different family to graphics to avoid contention
                if (queue[TRANSFER].family = findFamily(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT); queue[TRANSFER].family != UINT32_MAX) {
                    queue[TRANSFER].index = queueInfo[queue[TRANSFER].family].queueCount++;
                }
                else if (queue[TRANSFER].family = findFamily(VK_QUEUE_COMPUTE_BIT, 0); queue[TRANSFER].family != UINT32_MAX) {
                    queue[TRANSFER].index = queueInfo[queue[TRANSFER].family].queueCount++;
                }
                else {
                    queue[TRANSFER].family = queue[COMPUTE].family;
                    queue[TRANSFER].index = queue[COMPUTE].index;
                }
            }
    
            { // find present queue
                VkSurfaceKHR surface;
                GLFWwindow* window;
                { // create dummy window and surface
                    // glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // hide window
                    window = glfwCreateWindow(640, 480, "", nullptr, nullptr);
                    GLFW_ASSERT(window != nullptr);
                    
                    VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
                }
                
                auto testPresent = [&](uint32_t index) {
                    return glfwGetPhysicalDevicePresentationSupport(instance, gpu, index);
                };

                { // search queues for present support
                    if (testPresent(queue[GRAPHICS].family)) { // check if graphics family supports present
                        queue[PRESENT].family = queue[GRAPHICS].family;

                        // graphics family usually supports present, prefer separate queue for better throughput

                        if (queueInfo[queue[GRAPHICS].family].queueCount < queueFamilies[queue[GRAPHICS].family].queueCount) {
                            queue[PRESENT].index = queueInfo[queue[PRESENT].family].queueCount++;
                        }
                        else {
                            queue[PRESENT].index = queue[GRAPHICS].index;
                        }
                    }
                    else { 
                        // else prefer any separate queue
                        for (uint32_t i = 0; i < familyCount; ++i) {
                            if (queueInfo[i].queueCount < queueFamilies[i].queueCount && testPresent(i)) {
                                queue[PRESENT].family = i;
                                queue[PRESENT].index = queueInfo[queue[PRESENT].family].queueCount++;
                                break;
                            }
                        }
                        
                        if (queue[PRESENT].family == UINT32_MAX) { // fallback to shared present queue
                            if (testPresent(queue[COMPUTE].family)) {
                                queue[PRESENT].family = queue[COMPUTE].family;
                                queue[PRESENT].index = queue[COMPUTE].index;
                            }
                            else if (testPresent(queue[TRANSFER].family)) {
                                queue[PRESENT].family = queue[TRANSFER].family;
                                queue[PRESENT].index = queue[TRANSFER].index;
                            }
                            else {
                                throw std::runtime_error("failed to find family for PRESENT queue");
                            }
                        }
                    }
                    
                }
        
                { // destroy dummy window and surface
                    vkDestroySurfaceKHR(instance, surface, nullptr);
                    glfwDestroyWindow(window);
                }
            }
            
            // remove unrequested queues
            queueInfo.erase(std::remove_if(queueInfo.begin(), queueInfo.end(), [](auto& info) { 
                return info.queueCount == 0;
            }), queueInfo.end());

            { // get queue priorities
                // count queues
                uint32_t queueCount = 0;
                for (uint32_t i = 0; i < queueInfo.size(); ++i) {
                    queueCount += queueInfo[i].queueCount;
                }
                queuePriorities.resize(queueCount, 0.0f);

                // assign queue priority data for each family
                for (uint32_t f = 0, i = 0; f < familyCount; ++f, i += queueInfo[f].queueCount) {
                    queueInfo[f].pQueuePriorities = &queuePriorities[i];
                }
    
                // for each queue assign priority with max of shared set
                {
                    float& graphics_priority = *(queueInfo[queue[GRAPHICS].family].pQueuePriorities + queue[GRAPHICS].index - queuePriorities.data() + queuePriorities.data());
                    graphics_priority = std::max(graphics_priority, 0.8f);
    
                    float& compute_priority = *(queueInfo[queue[COMPUTE].family].pQueuePriorities + queue[COMPUTE].index - queuePriorities.data() + queuePriorities.data());
                    compute_priority = std::max(compute_priority, 0.6f);
                    
                    float& transfer_priority = *(queueInfo[queue[TRANSFER].family].pQueuePriorities + queue[TRANSFER].index - queuePriorities.data() + queuePriorities.data());
                    transfer_priority = std::max(transfer_priority, 0.4f);
    
                    float& present_priority = *(queueInfo[queue[PRESENT].family].pQueuePriorities + queue[PRESENT].index - queuePriorities.data() + queuePriorities.data());
                    present_priority = std::max(present_priority, 1.0f);
                }
                
            }
        }

        { // 
            VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES, 
                .pNext = nullptr,
                .descriptorBindingPartiallyBound = VK_TRUE,
                .runtimeDescriptorArray = VK_TRUE
            };
            VkPhysicalDeviceDynamicRenderingFeatures dynamic_features {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
                .pNext = &indexing_features,
                .dynamicRendering = VK_TRUE,
            };
            VkPhysicalDeviceFeatures core_features{
                .sampleRateShading = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
            };
            VkDeviceCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 
                .pNext = &dynamic_features, 
                .flags = 0, 
                .queueCreateInfoCount = static_cast<uint32_t>(queueInfo.size()),
                .pQueueCreateInfos = queueInfo.data(),
                .enabledLayerCount = 0,         // device layers are deprecated
                .ppEnabledLayerNames = nullptr,
                .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
                .ppEnabledExtensionNames = deviceExtensions.data(),
                .pEnabledFeatures = &core_features
            };
            VK_ASSERT(vkCreateDevice(gpu, &info, nullptr, &device));
        }
        
        { // get queues
            vkGetDeviceQueue(device, queue[PRESENT].family,  queue[PRESENT].index,  &queue[PRESENT].queue);
            vkGetDeviceQueue(device, queue[GRAPHICS].family, queue[GRAPHICS].index, &queue[GRAPHICS].queue);
            vkGetDeviceQueue(device, queue[COMPUTE].family,  queue[COMPUTE].index,  &queue[COMPUTE].queue);
            vkGetDeviceQueue(device, queue[TRANSFER].family, queue[TRANSFER].index, &queue[TRANSFER].queue);
        }

        { // create cmd pools
            VkCommandPoolCreateInfo info {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            };

            info.queueFamilyIndex = queue[GRAPHICS].family;
            vkCreateCommandPool(device, &info, nullptr, &queue[GRAPHICS].pool);

            info.queueFamilyIndex = queue[COMPUTE].family;
            vkCreateCommandPool(device, &info, nullptr, &queue[COMPUTE].pool);

            info.queueFamilyIndex = queue[TRANSFER].family;
            vkCreateCommandPool(device, &info, nullptr, &queue[TRANSFER].pool);

            info.queueFamilyIndex = queue[PRESENT].family;
            vkCreateCommandPool(device, &info, nullptr, &queue[PRESENT].pool);
        }
    }

    { // init memory allocators
        VmaAllocatorCreateInfo allocatorInfo {
            .physicalDevice = gpu,
            .device = device,
            .instance = instance,
            .vulkanApiVersion = VULKAN_VERSION,
        };
        VK_ASSERT(vmaCreateAllocator(&allocatorInfo, &allocator));
    }

}

Arawn::Engine::~Engine() {
    vkDestroyCommandPool(device, queue[GRAPHICS].pool, nullptr);
    vkDestroyCommandPool(device, queue[COMPUTE].pool, nullptr);
    vkDestroyCommandPool(device, queue[TRANSFER].pool, nullptr);
    vkDestroyCommandPool(device, queue[PRESENT].pool, nullptr);
    
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();

    instance = nullptr;
}
