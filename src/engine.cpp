#define ARAWN_IMPLEMENTATION
#include "engine.h"
#include <vector>
#include <algorithm>
#include <cstring>
#include "settings.h"
/*
1. init glfw
2. init vulkan instance
3. select GPU
4. init vulkan device
5. get queues
6. init command pools
7. init descriptor pools
*/

Engine::Engine() {
    std::vector<const char*> inst_layers =     { "VK_LAYER_KHRONOS_validation" };
    std::vector<const char*> device_layers =   { "VK_LAYER_KHRONOS_validation" };
    std::vector<const char*> inst_extensions = { VK_KHR_SURFACE_EXTENSION_NAME };
    std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME };

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
        app_info.apiVersion = VK_API_VERSION_1_2;
        app_info.pApplicationName = "Arawn";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "Arawn-Engine";
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
        if (settings.device != "") {
            for (uint32_t i = 0; i < count; ++i) {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(devices[i], &properties);
                
                if (strcmp(settings.device.c_str(), properties.deviceName) == 0) {
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

    { // check device feature support
        VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{};
        indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        indexing_features.pNext = nullptr;

        VkPhysicalDeviceFeatures2 supported{};
        supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        supported.pNext = &indexing_features;

        vkGetPhysicalDeviceFeatures2(gpu, &supported);

        if (!supported.features.samplerAnisotropy) 
            throw std::runtime_error("gpu does not support sampler anisotropy feature");

        if (!supported.features.sampleRateShading)
            throw std::runtime_error("gpu does not support sample rate shading");

        if (!indexing_features.descriptorBindingPartiallyBound)
            throw std::runtime_error("gpu does not support descriptor partial binding");
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

    uint32_t family_set_data[3];
    uint32_t family_set_count;

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
        
        VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{};
        indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        indexing_features.descriptorBindingPartiallyBound = VK_TRUE;

        VkPhysicalDeviceFeatures core_features{};
        core_features.samplerAnisotropy = VK_TRUE;
        core_features.sampleRateShading = VK_TRUE;

        VkDeviceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.pNext = &indexing_features;
        info.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
        info.pQueueCreateInfos = queues.data();
        info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        info.ppEnabledExtensionNames = device_extensions.data();
        info.enabledLayerCount = static_cast<uint32_t>(device_layers.size());
        info.ppEnabledLayerNames = device_layers.data();
        info.pEnabledFeatures = &core_features;

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
            VK_ASSERT(vkCreateCommandPool(device, &info, nullptr, cmd_pools + i));
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
        std::array<VkDescriptorPoolSize, 2> pool_sizes = { 
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 65535 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 65535 } // for textures
        };

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        info.poolSizeCount = pool_sizes.size();
        info.pPoolSizes = pool_sizes.data();
        info.maxSets = 65535;

        VK_ASSERT(vkCreateDescriptorPool(device, &info, nullptr, &descriptor_pool));
    }

    { // create sampler
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(engine.gpu, &properties);

        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.anisotropyEnable = VK_TRUE;
        info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        info.unnormalizedCoordinates = VK_FALSE;
        info.compareEnable = VK_FALSE;
        info.compareOp = VK_COMPARE_OP_ALWAYS;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.minLod = 0.0f;
        info.maxLod = MAX_MIPMAP_LEVEL;
        info.mipLodBias = 0.0f;

        VK_ASSERT(vkCreateSampler(engine.device, &info, nullptr, &sampler));
    }

    { // init descriptor set layouts
        { // camera layout
            std::array<VkDescriptorSetLayoutBinding, 1> set_layout_binding;

            VkDescriptorSetLayoutBinding& camera_binding = set_layout_binding[0];
            camera_binding.binding = 0;
            camera_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            camera_binding.descriptorCount = 1;
            camera_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            camera_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.pNext = nullptr;
            info.bindingCount = set_layout_binding.size();
            info.pBindings = set_layout_binding.data();

            VK_ASSERT(vkCreateDescriptorSetLayout(engine.device, &info, nullptr, &camera_layout));
        }

        
        { // transform layout
            std::array<VkDescriptorSetLayoutBinding, 1> set_layout_binding;

            VkDescriptorSetLayoutBinding& transform_binding = set_layout_binding[0];
            transform_binding.binding = 0;
            transform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            transform_binding.descriptorCount = 1;
            transform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            transform_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.pNext = nullptr;
            info.bindingCount = set_layout_binding.size();
            info.pBindings = set_layout_binding.data();
            
            VK_ASSERT(vkCreateDescriptorSetLayout(engine.device, &info, nullptr, &transform_layout));
        }

        { // material layout
            std::array<VkDescriptorSetLayoutBinding, 5> set_layout_binding;
            std::array<VkDescriptorBindingFlags, 5> binding_flags;

            VkDescriptorSetLayoutBinding& mat_binding = set_layout_binding[0];
            mat_binding.binding = 0;
            mat_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            mat_binding.descriptorCount = 1;
            mat_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            mat_binding.pImmutableSamplers = nullptr;

            VkDescriptorBindingFlags& mat_binding_flags = binding_flags[0];
            mat_binding_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

            VkDescriptorSetLayoutBinding& albedo_binding = set_layout_binding[1];
            albedo_binding.binding = 1;
            albedo_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            albedo_binding.descriptorCount = 1;
            albedo_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            albedo_binding.pImmutableSamplers = nullptr;

            VkDescriptorBindingFlags& albedo_binding_flags = binding_flags[1];
            albedo_binding_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

            VkDescriptorSetLayoutBinding& roughness_binding = set_layout_binding[2];
            roughness_binding.binding = 2;
            roughness_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            roughness_binding.descriptorCount = 1;
            roughness_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            roughness_binding.pImmutableSamplers = nullptr;

            VkDescriptorBindingFlags& roughness_binding_flags = binding_flags[2];
            roughness_binding_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

            VkDescriptorSetLayoutBinding& metallic_binding = set_layout_binding[3];
            metallic_binding.binding = 3;
            metallic_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            metallic_binding.descriptorCount = 1;
            metallic_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            metallic_binding.pImmutableSamplers = nullptr;

            VkDescriptorBindingFlags& metallic_binding_flags = binding_flags[3];
            metallic_binding_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

            VkDescriptorSetLayoutBinding& normal_binding = set_layout_binding[4];
            normal_binding.binding = 4;
            normal_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            normal_binding.descriptorCount = 1;
            normal_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            normal_binding.pImmutableSamplers = nullptr;

            VkDescriptorBindingFlags& normal_binding_flags = binding_flags[4];
            normal_binding_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

            VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info{};
            binding_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            binding_flags_info.pNext = nullptr;
            binding_flags_info.pBindingFlags = binding_flags.data();
            binding_flags_info.bindingCount = binding_flags.size();

            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.pNext = &binding_flags_info;
            info.bindingCount = set_layout_binding.size();
            info.pBindings = set_layout_binding.data();
            
            VK_ASSERT(vkCreateDescriptorSetLayout(engine.device, &info, nullptr, &material_layout));
        }

        { // lights/clusters layout
            std::array<VkDescriptorSetLayoutBinding, 2> set_layout_binding;
            
            VkDescriptorSetLayoutBinding& light_binding = set_layout_binding[0];
            light_binding.binding = 0;
            light_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            light_binding.descriptorCount = 1;
            light_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            light_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutBinding& cluster_binding = set_layout_binding[1];
            cluster_binding.binding = 1;
            cluster_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            cluster_binding.descriptorCount = 1;
            cluster_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            cluster_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.pNext = nullptr;
            info.bindingCount = set_layout_binding.size();
            info.pBindings = set_layout_binding.data();

            VK_ASSERT(vkCreateDescriptorSetLayout(engine.device, &info, nullptr, &light_layout));
            
        }
        
        { // deferred input attachments
            std::array<VkDescriptorSetLayoutBinding, 3> set_layout_binding;

            VkDescriptorSetLayoutBinding& albedo_attachment_binding = set_layout_binding[0];
            albedo_attachment_binding.binding = 0;
            albedo_attachment_binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            albedo_attachment_binding.descriptorCount = 1;
            albedo_attachment_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            albedo_attachment_binding.pImmutableSamplers = &sampler;

            VkDescriptorSetLayoutBinding& normal_attachment_binding = set_layout_binding[1];
            normal_attachment_binding.binding = 1;
            normal_attachment_binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            normal_attachment_binding.descriptorCount = 1;
            normal_attachment_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            normal_attachment_binding.pImmutableSamplers = &sampler;
            
            VkDescriptorSetLayoutBinding& specular_attachment_binding = set_layout_binding[2];
            specular_attachment_binding.binding = 2;
            specular_attachment_binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            specular_attachment_binding.descriptorCount = 1;
            specular_attachment_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            specular_attachment_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            info.pNext = nullptr;
            info.bindingCount = set_layout_binding.size();
            info.pBindings = set_layout_binding.data();

            VK_ASSERT(vkCreateDescriptorSetLayout(engine.device, &info, nullptr, &attachment_layout));
        }
        
    }
}

Engine::~Engine() {
    vkDestroySampler(engine.device, sampler, nullptr);

    vkDestroyDescriptorSetLayout(engine.device, camera_layout, nullptr);
    vkDestroyDescriptorSetLayout(engine.device, transform_layout, nullptr);
    vkDestroyDescriptorSetLayout(engine.device, material_layout, nullptr);
    vkDestroyDescriptorSetLayout(engine.device, light_layout, nullptr);
    vkDestroyDescriptorSetLayout(engine.device, attachment_layout, nullptr);
    
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

uint32_t Engine::memory_type_index(VkMemoryRequirements& requirements, VkMemoryPropertyFlags memory_property_flags) {
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(engine.gpu, &properties);

    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
    {
        if ((requirements.memoryTypeBits & (1 << i)) && 
            (properties.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags)
        {
            return i;
        }
    }

    throw std::runtime_error("no supported memory index found");
}

void log_error(std::string_view msg, std::source_location loc) {
    // I dont know if I've ever seen this function work...
    std::cout << loc.file_name() << ":" << loc.line() << " - " << msg.data();
}