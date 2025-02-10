
#define VK_IMPLEMENTATION
#include "engine.h"

Engine engine{ "ARAWN", "ARAWN-vulkan" };

Engine::Engine(const char* app_name, const char* engine_name) {
    std::vector<const char*> inst_layers = {
    "VK_LAYER_KHRONOS_validation"
    };
    std::vector<const char*> device_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    std::vector<const char*> inst_extensions {
        VK_KHR_SURFACE_EXTENSION_NAME
    };
    std::vector<const char*> device_extensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    
    { // init glfw
        assert(glfwInit() == GLFW_TRUE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    { // check instance layer support
        uint32_t count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        std::vector<VkLayerProperties> available(count);
        vkEnumerateInstanceLayerProperties(&count, available.data());
        
        // remove layer if not available 
        // TODO: warn about missing layer support
        inst_layers.erase(std::remove_if(inst_layers.begin(), inst_layers.end(), [&](const char* layer) {
            return std::find_if(available.begin(), available.end(), [&](const VkLayerProperties& p) { 
                return strcmp(layer, p.layerName) == 0; 
            }) == available.end();
        }), inst_layers.end());
    }

    { // get instance extensions support 
        uint32_t count;
        const char** exts = glfwGetRequiredInstanceExtensions(&count);
        std::copy(exts, exts + count, std::back_insert_iterator(inst_extensions));
    }

    { // init instance
        VkApplicationInfo app_info {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.apiVersion = VK_API_VERSION_1_0;     
        app_info.pApplicationName = app_name;
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = engine_name;
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

        VkInstanceCreateInfo inst_info {};
        inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        inst_info.pApplicationInfo = &app_info;
        inst_info.enabledExtensionCount = static_cast<uint32_t>(inst_extensions.size());
        inst_info.ppEnabledExtensionNames = inst_extensions.data();
        inst_info.enabledLayerCount = static_cast<uint32_t>(inst_layers.size());
        inst_info.ppEnabledLayerNames = inst_layers.data();
        
        VK_ASSERT(vkCreateInstance(&inst_info, nullptr, &instance));
    }

    { // select gpu
        uint32_t count = 0;
        VK_ASSERT(vkEnumeratePhysicalDevices(instance, &count, nullptr));

        if (count == 0)
            throw std::runtime_error("failed to find physical device with vulkan support");

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(instance, &count, devices.data());

        gpu = devices[1]; // 0=intel, 1=nvidia
    }

    { // check device layer support
        uint32_t count;
        VK_ASSERT(vkEnumerateDeviceLayerProperties(gpu, &count, nullptr));
        
        std::vector<VkLayerProperties> available(count);
        VK_ASSERT(vkEnumerateDeviceLayerProperties(gpu, &count, available.data()));

        // remove layer if not available
        device_layers.erase(std::remove_if(device_layers.begin(), device_layers.end(), [&](const char* layer) {
            return std::find_if(available.begin(), available.end(), [&](const VkLayerProperties& p) { 
                return strcmp(layer, p.layerName) == 0; 
            }) == available.end();
        }), device_layers.end());
    }

    { // check device extension support
        uint32_t count;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr);

        std::vector<VkExtensionProperties> available(count);
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, available.data());
        
        for (const char* layer : device_extensions) {
            auto it = std::find_if(available.begin(), available.end(), 
                [&](const VkExtensionProperties& p) { return strcmp(layer, p.extensionName) == 0; });
            
            if (it == available.end())
                throw std::runtime_error("device extension not available");
        }
    }

    { // select queues
        uint32_t family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, NULL);
        
        std::vector<VkQueueFamilyProperties> prop(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, prop.data());
    
        graphic_family = family_count;
        compute_family = family_count;
        present_family = family_count;

        { // find graphics queue
            for (uint32_t i = 0; i < family_count; ++i) {
                if (prop[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphic_family = i;
                    --prop[i].queueCount;
                    break;
                }
            }
            
            assert(graphic_family != family_count); // no graphics, should be impossible
        }

        { // find compute queue
            if (prop[graphic_family].queueFlags & VK_QUEUE_COMPUTE_BIT) // queue sharing
                compute_family = graphic_family;

            for (uint32_t i = 0; i < family_count; ++i) {
                if (i == graphic_family) continue;
                if (prop[i].queueCount > 0 && prop[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    compute_family = i;
                    --prop[i].queueCount;
                    break;
                }
            }

            assert(compute_family != family_count); // no compute
        }
        
        VkSurfaceKHR surface;
        GLFWwindow* window;
        { // create dummy window and surface
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // hide window
            window = glfwCreateWindow(640, 480, "", NULL, NULL);

            assert(window != nullptr);
            
            VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
        }

        { // find present queue
            VkBool32 support = VK_FALSE;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(gpu, graphic_family, surface, &support) == VK_SUCCESS && support == VK_TRUE)
                present_family = graphic_family;
            
            else if (vkGetPhysicalDeviceSurfaceSupportKHR(gpu, compute_family, surface, &support) == VK_SUCCESS && support == VK_TRUE)
                present_family = compute_family;
            
            for (uint32_t i = 0; i < family_count; ++i) {
                if (prop[i].queueCount > 0 && vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &support) == VK_SUCCESS && support == VK_TRUE) {
                    present_family = i;
                    --prop[i].queueCount;
                    break;
                }
            }

            assert(present_family != family_count);
        }

        { // destroy dummy window and surface
            vkDestroySurfaceKHR(instance, surface, nullptr);
            glfwDestroyWindow(window);
        }
    }

    { // init device
        // get unique queues
        uint32_t family_set[3];
        uint32_t family_set_count = get_queue_family_set(family_set);
        
        float priority = 1.0f; // priority
        std::vector<VkDeviceQueueCreateInfo> queues;
        {
            VkDeviceQueueCreateInfo info {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.pQueuePriorities = &priority;
            
            for (uint32_t i = 0; i < family_set_count; ++i) {
                info.queueFamilyIndex = family_set[i];
                info.queueCount = 1;
                queues.push_back(info);
            }
        }
        
        VkDeviceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
        info.pQueueCreateInfos = queues.data();
        info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        info.ppEnabledExtensionNames = device_extensions.data();
        info.enabledLayerCount = static_cast<uint32_t>(device_layers.size());
        info.ppEnabledLayerNames = device_layers.data();
        VK_ASSERT(vkCreateDevice(gpu, &info, NULL, &device));
        
        vkGetDeviceQueue(device, graphic_family, 0, &graphic_queue);
        vkGetDeviceQueue(device, compute_family, 0, &compute_queue);
        vkGetDeviceQueue(device, present_family, 0, &present_queue);
    }
}

Engine::~Engine() {
    if (instance == VK_NULL_HANDLE) return;
    
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();
}

uint32_t Engine::get_queue_family_set(uint32_t* family_set) const {
    if (family_set == nullptr) {
        return 1 + (graphic_family != compute_family) + (graphic_family != present_family && compute_family != present_family);
    }

    uint32_t count = 1;
    family_set[0] = graphic_family;

    if (graphic_family != compute_family)
        family_set[count++] = compute_family; 
    
    if (graphic_family != present_family && compute_family != present_family) 
        family_set[count++] = present_family;

    return count;
}