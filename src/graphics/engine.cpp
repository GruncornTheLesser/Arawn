#define ARAWN_IMPLEMENTATION
#include "engine.h"
#include "settings.h"
#include <vector>
#include <algorithm>
#include <cstring>
#include <fstream>
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
        VkApplicationInfo app_info {
            VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, 
            "Arawn", VK_MAKE_VERSION(1, 0, 0), 
            "Arawn-Engine", VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_2
        };

        VkInstanceCreateInfo inst_info {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, &app_info, 
            static_cast<uint32_t>(inst_layers.size()), inst_layers.data(),
            static_cast<uint32_t>(inst_extensions.size()), inst_extensions.data()
        };
        
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
        VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES, nullptr };
        VkPhysicalDeviceFeatures2 supported{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &indexing_features };
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
        
        std::vector<VkQueueFamilyProperties> properties(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, properties.data());
    
        graphics.family = family_count;
        compute.family = family_count;
        present.family = family_count;

        { // find graphics queue
            for (uint32_t i = 0; i < family_count; ++i) {
                if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphics.family = i;
                    --properties[i].queueCount;
                    break;
                }
            }
            
            if (graphics.family == family_count) throw std::exception(); // no graphics, should be impossible
        }

        { // find compute queue
            if (properties[graphics.family].queueFlags & VK_QUEUE_COMPUTE_BIT) // queue sharing
                compute.family = graphics.family;

            for (uint32_t i = 0; i < family_count; ++i) {
                if (i == graphics.family) continue;
                if (properties[i].queueCount > 0 && properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    compute.family = i;
                    --properties[i].queueCount;
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
                if (properties[i].queueCount > 0 && vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &support) == VK_SUCCESS && support == VK_TRUE) {
                    present.family = i;
                    --properties[i].queueCount;
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

    std::array<uint32_t, 3> queue_families = { graphics.family, compute.family, present.family };
    std::ranges::sort(queue_families);
    auto unique_queue_families = std::ranges::subrange(queue_families.begin(), std::ranges::unique(queue_families).begin());

    { // init device
        float priority = 1.0f; // priority
        std::vector<VkDeviceQueueCreateInfo> queues;
        for (uint32_t family : unique_queue_families) {
            queues.emplace_back(
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, 
                family, 1, &priority
            );
        }
        
        VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES };
        indexing_features.descriptorBindingPartiallyBound = VK_TRUE;

        VkPhysicalDeviceFeatures core_features{ };
        core_features.samplerAnisotropy = VK_TRUE;
        core_features.sampleRateShading = VK_TRUE;

        VkDeviceCreateInfo info{ 
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, &indexing_features, 0, 
            static_cast<uint32_t>(queues.size()), queues.data(),
            static_cast<uint32_t>(device_layers.size()), device_layers.data(), 
            static_cast<uint32_t>(device_extensions.size()), device_extensions.data(),
            &core_features
        };

        VK_ASSERT(vkCreateDevice(gpu, &info, nullptr, &device));
    }

    { // get queues
        vkGetDeviceQueue(device, graphics.family, 0, &graphics.queue);
        vkGetDeviceQueue(device, compute.family, 0, &compute.queue);
        vkGetDeviceQueue(device, present.family, 0, &present.queue);
    }

    { // init command pools
        std::array<VkCommandPool, 3> cmd_pools;
        VkCommandPoolCreateInfo info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT };
        for (uint32_t i = 0; i < unique_queue_families.size(); ++i) {
            info.queueFamilyIndex = unique_queue_families[i];
            VK_ASSERT(vkCreateCommandPool(device, &info, nullptr, &cmd_pools[i]));
        }

        // get command pool from queue family
        graphics.pool = cmd_pools[std::ranges::find(unique_queue_families, graphics.family) - unique_queue_families.begin()];
        compute.pool = cmd_pools[std::ranges::find(unique_queue_families, compute.family) - unique_queue_families.begin()];
    }

    { // init descriptor pools
        std::array<VkDescriptorPoolSize, 3> pool_sizes = { 
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 255 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 65535 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 65535 } // for textures
        };

        VkDescriptorPoolCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT };
        info.poolSizeCount = pool_sizes.size();
        info.pPoolSizes = pool_sizes.data();
        info.maxSets = 65535;

        VK_ASSERT(vkCreateDescriptorPool(device, &info, nullptr, &descriptor_pool));
    }

    { // create sampler
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(engine.gpu, &properties);

        VkSamplerCreateInfo info{
            VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, nullptr, 0, 
            VK_FILTER_LINEAR, VK_FILTER_LINEAR, 
            VK_SAMPLER_MIPMAP_MODE_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_REPEAT, 
            VK_SAMPLER_ADDRESS_MODE_REPEAT, 
            VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.0f, 
            VK_FALSE, 0.0f, 
            VK_FALSE, VK_COMPARE_OP_NEVER, 0.0f, MAX_MIPMAP_LEVEL, 
            VK_BORDER_COLOR_INT_OPAQUE_BLACK, VK_FALSE
        };
 
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

            VkDescriptorSetLayoutCreateInfo info{ 
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 
                set_layout_binding.size(), set_layout_binding.data() 
            };
            
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

            VkDescriptorSetLayoutCreateInfo info{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0,
                set_layout_binding.size(), set_layout_binding.data()
            };
            
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

            VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, nullptr, 
                binding_flags.size(), binding_flags.data()
            };

            VkDescriptorSetLayoutCreateInfo info{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, &binding_flags_info, 0,
                set_layout_binding.size(), set_layout_binding.data()
            };
            
            VK_ASSERT(vkCreateDescriptorSetLayout(engine.device, &info, nullptr, &material_layout));
        }

        { // lights/clusters layout
            std::array<VkDescriptorSetLayoutBinding, 4> set_layout_binding;
            
            VkDescriptorSetLayoutBinding& light_binding = set_layout_binding[0];
            light_binding.binding = 0;
            light_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            light_binding.descriptorCount = 1;
            light_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            light_binding.pImmutableSamplers = nullptr;
            
            VkDescriptorSetLayoutBinding& frustum_binding = set_layout_binding[1];
            frustum_binding.binding = 1;
            frustum_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            frustum_binding.descriptorCount = 1;
            frustum_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            frustum_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutBinding& cluster_binding = set_layout_binding[2];
            cluster_binding.binding = 2;
            cluster_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            cluster_binding.descriptorCount = 1;
            cluster_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            cluster_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutBinding& depth_binding = set_layout_binding[3];
            depth_binding.binding = 3;
            depth_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            depth_binding.descriptorCount = 1;
            depth_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
            depth_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutCreateInfo info{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 
                set_layout_binding.size(), set_layout_binding.data() 
            };
            
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
            
            VkDescriptorSetLayoutBinding& position_attachment_binding = set_layout_binding[2];
            position_attachment_binding.binding = 2;
            position_attachment_binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            position_attachment_binding.descriptorCount = 1;
            position_attachment_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            position_attachment_binding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutCreateInfo info{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0,
                set_layout_binding.size(), set_layout_binding.data()
            };

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

VkShaderModule Engine::create_shader(std::filesystem::path fp) const {
    std::ifstream file(fp, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("could not open file: " + fp.string());

    std::vector<char> buffer(static_cast<std::size_t>(file.tellg()));
    file.seekg(0);
    file.read(buffer.data(), buffer.size());
    file.close();

    VkShaderModuleCreateInfo info {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, 
        buffer.size(), reinterpret_cast<uint32_t*>(buffer.data())
    };

    VkShaderModule shader;
    VK_ASSERT(vkCreateShaderModule(engine.device, &info, NULL, &shader));
    return shader;
}


void log_error(std::string_view msg, std::source_location loc) {
    // I dont know if I've ever seen this function work...
    std::cout << loc.file_name() << ":" << loc.line() << " - " << msg.data();
}