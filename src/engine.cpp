#define VK_IMPLEMENTATION
#include "engine.h"
#include <vector>
#include <algorithm>
#include <cstring>

/*
1. init glfw
2. init vulkan instance
3. select GPU
4. init vulkan device
5. get queues
6. init command pools
7. init descriptor pools
*/

Engine::Engine(const char* app_name, const char* engine_name, const char* device_name) {
    std::vector<const char*> inst_layers =     { "VK_LAYER_KHRONOS_validation"   };
    std::vector<const char*> device_layers =   { "VK_LAYER_KHRONOS_validation"   };
    std::vector<const char*> inst_extensions   { VK_KHR_SURFACE_EXTENSION_NAME   };
    std::vector<const char*> device_extensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    { // init glfw
        GLFW_ASSERT(glfwInit() == GLFW_TRUE);
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
        
        uint32_t index = 0;
        if (engine_name != nullptr) {
            for (uint32_t i = 0; i < count; ++i) {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(devices[i], &properties);
                
                if (strcmp(device_name, properties.deviceName) == 0) {
                    index = i;
                    break;
                }
            }
        }
        gpu = devices[index]; // 0=intel, 1=nvidia
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
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, nullptr);
        
        std::vector<VkQueueFamilyProperties> prop(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, prop.data());
    
        graphics.family = family_count;
        compute.family = family_count;
        present.family = family_count;

        { // find graphics queue
            for (uint32_t i = 0; i < family_count; ++i) {
                if (prop[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphics.family = i;
                    --prop[i].queueCount;
                    break;
                }
            }
            
            if (graphics.family == family_count) throw std::exception(); // no graphics, should be impossible
        }

        { // find compute queue
            if (prop[graphics.family].queueFlags & VK_QUEUE_COMPUTE_BIT) // queue sharing
                compute.family = graphics.family;

            for (uint32_t i = 0; i < family_count; ++i) {
                if (i == graphics.family) continue;
                if (prop[i].queueCount > 0 && prop[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    compute.family = i;
                    --prop[i].queueCount;
                    break;
                }
            }

            if (compute.family == family_count) throw std::exception();
        }
        
        VkSurfaceKHR surface;
        GLFWwindow* window;
        { // create dummy window and surface
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // hide window
            window = glfwCreateWindow(640, 480, "", nullptr, nullptr);
            GLFW_ASSERT(window != nullptr);
            
            VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
        }

        { // find present queue
            VkBool32 support = VK_FALSE;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(gpu, graphics.family, surface, &support) == VK_SUCCESS && support == VK_TRUE)
                present.family = graphics.family;
            
            else if (vkGetPhysicalDeviceSurfaceSupportKHR(gpu, compute.family, surface, &support) == VK_SUCCESS && support == VK_TRUE)
                present.family = compute.family;
            
            for (uint32_t i = 0; i < family_count; ++i) {
                if (prop[i].queueCount > 0 && vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &support) == VK_SUCCESS && support == VK_TRUE) {
                    present.family = i;
                    --prop[i].queueCount;
                    break;
                }
            }

            if (present.family == family_count) throw std::exception();
        }

        { // destroy dummy window and surface
            vkDestroySurfaceKHR(instance, surface, nullptr);
            glfwDestroyWindow(window);
        }
    }

    { // get queue family set
        uint32_t i = 0;
        family_set_data[i] = graphics.family;

        if (graphics.family != compute.family)
            family_set_data[++i] = compute.family;
        
        if (graphics.family != present.family && compute.family != present.family) 
            family_set_data[++i] = present.family;
        
        family_set_count = ++i;
    }
    
    { // init device
        float priority = 1.0f; // priority
        std::vector<VkDeviceQueueCreateInfo> queues;
        {
            VkDeviceQueueCreateInfo info {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.pQueuePriorities = &priority;
            
            for (uint32_t i = 0; i < family_set_count; ++i) {
                info.queueFamilyIndex = family_set_data[i];
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
        VK_ASSERT(vkCreateDevice(gpu, &info, nullptr, &device));
    }

    { // get queues
        vkGetDeviceQueue(device, graphics.family, 0, &graphics.queue);
        vkGetDeviceQueue(device, compute.family, 0, &compute.queue);
        vkGetDeviceQueue(device, present.family, 0, &present.queue);
    }

    { // init command pools
        VkCommandPool cmd_pools[3];
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        for (uint32_t i = 0; i < family_set_count; ++i) {
            info.queueFamilyIndex = family_set_data[i];
            vkCreateCommandPool(device, &info, nullptr, cmd_pools + i);
        }

        // get command pool from queue family 
        uint32_t i = 0;
        graphics.pool = cmd_pools[i];

        if (compute.family != graphics.family) ++i;
        compute.pool = cmd_pools[i];

        //if (present.family != graphics.family && 
        //    present.family != compute.family) ++i;
        //present.pool = cmd_pools[i];
    }

    { // init descriptor pools
        VkDescriptorPoolSize pool_sizes[3] = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256 }, // for textures
        };

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.poolSizeCount = 3;
        info.pPoolSizes = pool_sizes;
        info.maxSets = 256;

        VK_ASSERT(vkCreateDescriptorPool(device, &info, nullptr, &descriptor_pool));
    }
    
    { // select formats
        VkFormat depth_formats[3]  = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }; 
        format.attachment.depth = select_image_format({ depth_formats, 3 }, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        // ...
    
    }
}

Engine::~Engine() {
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

    vkDestroyCommandPool(device, graphics.pool, nullptr);

    if (compute.family != graphics.family)
        vkDestroyCommandPool(device, compute.pool, nullptr);

    //if (present.family != graphics.family && present.family != compute.family) 
    //    vkDestroyCommandPool(device, present.pool, nullptr);

    vkDestroyDevice(device, nullptr);

    vkDestroyInstance(instance, nullptr);

    glfwTerminate();
}

VkFormat Engine::select_buffer_format(std::span<VkFormat> formats, VkFormatFeatureFlags features) const {
    VkFormatProperties properties;
    for (VkFormat format : formats) {
        vkGetPhysicalDeviceFormatProperties(engine.gpu, format, &properties);
        if ((properties.bufferFeatures & features) != 0) 
            return format;
    }
    throw std::runtime_error("no specified format supported");
}

VkFormat Engine::select_image_format(std::span<VkFormat> formats, VkFormatFeatureFlags features) const {
    VkFormatProperties properties;
   for (VkFormat format : formats) {
        vkGetPhysicalDeviceFormatProperties(engine.gpu, format, &properties);
        if ((properties.optimalTilingFeatures & features) != 0) 
            return format;
    }
    throw std::runtime_error("no specified format supported");
}

uint32_t Engine::get_memory_index(uint32_t type_bits, VkMemoryPropertyFlags flags) const {
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(engine.gpu, &properties);

    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((type_bits & (1 << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    throw std::exception();
}

void log_error(std::string_view msg, std::source_location loc) {
    std::cout << loc.file_name() << ":" << loc.line() << " - " << msg.data();
}

