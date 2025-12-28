#define ARAWN_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include <graphics/engine.h>
#include <core/settings.h>

#include <vector>
#include <algorithm>
#include <cstring>


using namespace Arawn;

#define VULKAN_VERSION VK_API_VERSION_1_2

Engine::Engine()
{
    #ifdef ARAWN_DEBUG
    std::vector<const char*> instanceLayers = { "VK_LAYER_KHRONOS_validation" };
    std::vector<const char*> deviceLayers = { "VK_LAYER_KHRONOS_validation" };
    #endif

    std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    { // init glfw
        GLFW_ASSERT(glfwInit() == GLFW_TRUE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    { // print vulkan version
        uint32_t version;
        VK_ASSERT(vkEnumerateInstanceVersion(&version));
        uint32_t major = VK_VERSION_MAJOR(version);
        uint32_t minor = VK_VERSION_MINOR(version);
        uint32_t patch = VK_VERSION_PATCH(version);
        LOG("vulkan version: " << major << "." << minor << "." << patch);
    }
    
    #ifdef ARAWN_DEBUG
    { // remove unsupported layers support
        uint32_t count;
        VK_ASSERT(vkEnumerateInstanceLayerProperties(&count, nullptr));
        
        std::vector<VkLayerProperties> available(count);
        VK_ASSERT(vkEnumerateInstanceLayerProperties(&count, available.data()));

        // remove layer if not supported 
        instanceLayers.erase(std::remove_if(instanceLayers.begin(), instanceLayers.end(), [&](const char* layer)
        {
            return std::find_if(available.begin(), available.end(), [&](const VkLayerProperties& properties)
            { 
                return strcmp(layer, properties.layerName) == 0; 
            }) == available.end();
        }), instanceLayers.end());
    }
    #endif

    { // get glfw vulkan extensions 
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
        
        for (const char* extension : instanceExtensions)
        {
            auto it = std::find_if(available.begin(), available.end(), 
                [&](const VkExtensionProperties& p)
                { return strcmp(extension, p.extensionName) == 0; });
            
            if (it == available.end())
                throw std::runtime_error("instance " + std::string(extension) + " extension not supported");
        }
    }

    { // init instance
        VkApplicationInfo app_info {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, 
            .pNext = nullptr, 
            .pApplicationName = "Arawn", 
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0), 
            .pEngineName = "Arawn-Engine", 
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VULKAN_VERSION
        };

        VkInstanceCreateInfo inst_info {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 
            .pNext = nullptr,
            .flags = 0, 
            .pApplicationInfo = &app_info,
        #ifdef ARAWN_DEBUG
            .enabledLayerCount = static_cast<uint32_t>(instanceLayers.size()),
            .ppEnabledLayerNames = instanceLayers.data(),
        #else
            .enabledLayerCount = nullptr,
            .ppEnabledLayerNames = 0,
        #endif
            .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()), 
            .ppEnabledExtensionNames = instanceExtensions.data()
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
        
        uint32_t index = 0; // defaults to first
        std::string deviceName = settings.get<DeviceName>();
        if (deviceName != "")
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(devices[i], &properties);
                
                if (strcmp(deviceName.c_str(), properties.deviceName) == 0)
                {
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
    
            LOG("gpu: " << properties.deviceName)
        }
        #endif
    }

    #ifdef ARAWN_DEBUG
    { // check device layer support
        uint32_t count;
        VK_ASSERT(vkEnumerateDeviceLayerProperties(gpu, &count, nullptr));
        
        std::vector<VkLayerProperties> available(count);
        VK_ASSERT(vkEnumerateDeviceLayerProperties(gpu, &count, available.data()));

        // remove layer if not available
        deviceLayers.erase(std::remove_if(deviceLayers.begin(), deviceLayers.end(), [&](const char* layer)
        {
            return std::find_if(available.begin(), available.end(), [&](const VkLayerProperties& p)
            { 
                return strcmp(layer, p.layerName) == 0; 
            }) == available.end();
        }), deviceLayers.end());
    }
    #endif

    { // check device extension support
        uint32_t count;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr);

        std::vector<VkExtensionProperties> available(count);
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, available.data());
        
        for (const char* extension : deviceExtensions)
        {
            auto it = std::find_if(available.begin(), available.end(), 
                [&](const VkExtensionProperties& p)
                { return strcmp(extension, p.extensionName) == 0; });
            
            if (it == available.end())
                throw std::runtime_error("device extension not supported");
        }
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

        if (!supported.features.sampleRateShading)
            throw std::runtime_error("gpu does not support sample rate shading");

        if (!indexingFeatures.descriptorBindingPartiallyBound || !indexingFeatures.runtimeDescriptorArray)
            throw std::runtime_error("gpu does not support bindless rendering");
    }

    { // init device
        uint32_t familyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
        
        std::vector<VkDeviceQueueCreateInfo> queues(familyCount, VkDeviceQueueCreateInfo{ 
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = 0,
            .queueCount = 0,
            .pQueuePriorities = nullptr
        });

        for (uint32_t i = 0; i < queues.size(); ++i)
        {
            queues[i].queueFamilyIndex = i;
        }

        uint32_t graphicsIndex = UINT32_MAX;
        uint32_t computeIndex = UINT32_MAX;
        uint32_t transferIndex = UINT32_MAX;
        uint32_t asyncIndex = UINT32_MAX;
        uint32_t presentIndex = UINT32_MAX;

        { // select queues
            std::vector<VkQueueFamilyProperties> queueFamilies(familyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, queueFamilies.data());

            family[GRAPHICS] = UINT32_MAX;
            family[COMPUTE] = UINT32_MAX;
            family[TRANSFER] = UINT32_MAX;
            family[ASYNC] = UINT32_MAX;
            family[PRESENT] = UINT32_MAX;
            
            auto findFamilyIndex = [&](uint32_t include, uint32_t exclude) {
                for (uint32_t i = 0; i < familyCount; ++i)
                {
                    if (queues[i].queueCount < queueFamilies[i].queueCount && (queueFamilies[i].queueFlags & include) == include && !(queueFamilies[i].queueFlags & exclude))
                    {
                        return i;
                    }
                }
                return UINT32_MAX;
            };
    
            // find graphics queue
            // graphics queue is used as a fallback for any work, must implement graphics, prefer implememt compute and transfer 
            {
                if (family[GRAPHICS] = findFamilyIndex(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, 0); family[GRAPHICS] != UINT32_MAX) 
                {
                    graphicsIndex = queues[family[GRAPHICS]].queueCount++;
                }
                else if (family[GRAPHICS] = findFamilyIndex(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0); family[GRAPHICS] != UINT32_MAX) 
                {
                    graphicsIndex = queues[family[GRAPHICS]].queueCount++;
                }
                else if (family[GRAPHICS] = findFamilyIndex(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT, 0); family[GRAPHICS] != UINT32_MAX) 
                {
                    graphicsIndex = queues[family[GRAPHICS]].queueCount++;
                }
                else if (family[GRAPHICS] = findFamilyIndex(VK_QUEUE_GRAPHICS_BIT, 0), family[GRAPHICS] != UINT32_MAX) 
                {
                    graphicsIndex = queues[family[GRAPHICS]].queueCount++;
                } else
                {
                    throw std::runtime_error("failed to find family for graphics queue");
                }
            }
    
            // find compute queue
            // prefer shared compute/graphics shared family to reduce inter-family synchronizations
            {
                if (queueFamilies[family[GRAPHICS]].queueCount > queues[family[GRAPHICS]].queueCount && queueFamilies[family[GRAPHICS]].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    family[COMPUTE] = family[GRAPHICS];
                    computeIndex = queues[family[COMPUTE]].queueCount++;
                } 
                else if (family[COMPUTE] = findFamilyIndex(VK_QUEUE_COMPUTE_BIT, 0); family[COMPUTE] != UINT32_MAX)
                {
                    computeIndex = queues[family[COMPUTE]].queueCount++;
                }
                else if (queueFamilies[family[GRAPHICS]].queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    family[COMPUTE] = family[GRAPHICS];
                    computeIndex = 0;
                }
                else
                {
                    throw std::runtime_error("failed to find family for compute queue");
                }
            }
    
            // find async family
            // prefer exclusive queue in different family to graphics to avoid contention
            {
                if (family[ASYNC] = findFamilyIndex(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT); family[ASYNC] != UINT32_MAX) {
                    asyncIndex = queues[family[ASYNC]].queueCount++;
                } 
                else if (family[ASYNC] = findFamilyIndex(VK_QUEUE_COMPUTE_BIT, 0); family[ASYNC] != UINT32_MAX) 
                {
                    asyncIndex = queues[family[ASYNC]].queueCount++;
                }
                else
                {
                    family[ASYNC] = family[COMPUTE];
                    asyncIndex = computeIndex;
                }
            }
            
            // find transfer family
            {
                if (family[TRANSFER] = findFamilyIndex(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT); family[TRANSFER] != UINT32_MAX)
                {
                    transferIndex = queues[family[TRANSFER]].queueCount++;
                } 
                else if (family[TRANSFER] = findFamilyIndex(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT); family[TRANSFER] != UINT32_MAX)
                {
                    transferIndex = queues[family[TRANSFER]].queueCount++;
                }
                else if (family[TRANSFER] = findFamilyIndex(VK_QUEUE_TRANSFER_BIT, 0); family[TRANSFER] != UINT32_MAX)
                {
                    transferIndex = queues[family[TRANSFER]].queueCount++;
                }
                else if (queueFamilies[family[ASYNC]].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    family[TRANSFER] = family[ASYNC];
                    transferIndex = asyncIndex;
                }
                else if (queueFamilies[family[COMPUTE]].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    family[TRANSFER] = family[COMPUTE];
                    transferIndex = computeIndex;
                } 
                else if (queueFamilies[family[GRAPHICS]].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    family[TRANSFER] = family[GRAPHICS];
                    transferIndex = graphicsIndex;
                } 
                else
                {
                    throw std::runtime_error("failed to find family for transfer queue");
                }
            }
    
            { // find present queue
                VkSurfaceKHR surface;
                GLFWwindow* window;
                { // create dummy window and surface
                    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // hide window
                    window = glfwCreateWindow(640, 480, "", nullptr, nullptr);
                    GLFW_ASSERT(window != nullptr);
                    
                    VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
                }
                
                { // search queues for properties
                    VkBool32 support = VK_FALSE;
                    
                    // check if graphics family supports present
                    if (queues[family[GRAPHICS]].queueCount < queueFamilies[family[GRAPHICS]].queueCount && vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family[GRAPHICS], surface, &support) == VK_SUCCESS && support == VK_TRUE)
                    {
                        family[PRESENT] = family[GRAPHICS];
                        presentIndex = queues[family[PRESENT]].queueCount++;
    
                    }
                    else // else prefer any separate queue
                    {
                        for (uint32_t i = 0; i < familyCount; ++i)
                        {
                            if (queues[i].queueCount < queueFamilies[i].queueCount && vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &support) == VK_SUCCESS && support == VK_TRUE)
                            {
                                family[PRESENT] = i;
                                presentIndex = queues[family[PRESENT]].queueCount++;
                                break;
                            }
                        }
                    }
    
                    // fallback to shared present queue
                    if (family[PRESENT] == UINT32_MAX) {
                        if (vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family[GRAPHICS], surface, &support) == VK_SUCCESS && support == VK_TRUE) 
                        {
                            family[PRESENT] = family[GRAPHICS];
                            presentIndex = graphicsIndex;
                        } 
                        else if (vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family[COMPUTE], surface, &support) == VK_SUCCESS && support == VK_TRUE)
                        {
                            family[PRESENT] = family[COMPUTE];
                            presentIndex = computeIndex;
                        }
                        else if (vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family[TRANSFER], surface, &support) == VK_SUCCESS && support == VK_TRUE)
                        {
                            family[PRESENT] = family[TRANSFER];
                            presentIndex = transferIndex;
                        } 
                        else if (vkGetPhysicalDeviceSurfaceSupportKHR(gpu, family[ASYNC], surface, &support) == VK_SUCCESS && support == VK_TRUE)
                        {
                            family[PRESENT] = family[ASYNC];
                            presentIndex = asyncIndex;
                        }
                        else
                        {
                            throw std::runtime_error("failed to find family for present queue");
                        }
                    }
                }
        
                { // destroy dummy window and surface
                    vkDestroySurfaceKHR(instance, surface, nullptr);
                    glfwDestroyWindow(window);
                }
            }

            // remove unrequested queues
            queues.erase(std::remove_if(queues.begin(), queues.end(), [](auto& info) { return info.queueCount == 0; }), queues.end());
        }
        
        uint32_t queueCount = 0;
        { // get queue count
            for (uint32_t i = 0; i < queues.size(); ++i)
            {
                queueCount += queues[i].queueCount;
            }
        }

        std::vector<float> priorities(queueCount, 0.0f);
        { // assign queue priorities
            for (uint32_t i = 0, j = 0; i < queues.size(); ++i, j += queues[i].queueCount)
            { 
                queues[i].pQueuePriorities = &priorities[j];
            }
        
            { // queue priorities
                std::min(1.0f, queues[family[PRESENT]].pQueuePriorities[presentIndex]);
                std::min(0.8f, queues[family[GRAPHICS]].pQueuePriorities[graphicsIndex]);
                std::min(0.6f, queues[family[COMPUTE]].pQueuePriorities[computeIndex]);
                std::min(0.4f, queues[family[ASYNC]].pQueuePriorities[asyncIndex]);
                std::min(0.2f, queues[family[TRANSFER]].pQueuePriorities[transferIndex]);
            }
        }

        VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES, 
            .pNext = nullptr,
            .descriptorBindingPartiallyBound = VK_TRUE,
            .runtimeDescriptorArray = VK_TRUE
        };

        VkPhysicalDeviceFeatures coreFeatures{
            .sampleRateShading = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
        };

        VkDeviceCreateInfo info{ 
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 
            .pNext = &indexingFeatures, 
            .flags = 0, 
            .queueCreateInfoCount = static_cast<uint32_t>(queues.size()),
            .pQueueCreateInfos = queues.data(),
        #ifdef ARAWN_DEBUG
            .enabledLayerCount = static_cast<uint32_t>(deviceLayers.size()),
            .ppEnabledLayerNames = deviceLayers.data(),
        #else
            .enabledLayerCount = nullptr,
            .ppEnabledLayerNames = 0,
        #endif
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = &coreFeatures
        };

        VK_ASSERT(vkCreateDevice(gpu, &info, nullptr, &device));
        
        { // get queues
            vkGetDeviceQueue(device, family[PRESENT], presentIndex, &queue[PRESENT]);
            vkGetDeviceQueue(device, family[GRAPHICS], graphicsIndex, &queue[GRAPHICS]);
            vkGetDeviceQueue(device, family[COMPUTE], computeIndex, &queue[COMPUTE]);
            vkGetDeviceQueue(device, family[ASYNC], asyncIndex, &queue[ASYNC]);
            vkGetDeviceQueue(device, family[TRANSFER], transferIndex, &queue[TRANSFER]);
        }
    }

    { // init memory allocators
        VmaAllocatorCreateInfo allocatorInfo {
            .physicalDevice = gpu,
            .device = device,
            .instance = instance,
            .vulkanApiVersion = VULKAN_VERSION,
        };
        vmaCreateAllocator(&allocatorInfo, &allocator);
    }
    
    


    /*
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
    */
}

Engine::~Engine()
{
    /*
    vkDestroySampler(engine.device, sampler, nullptr);

    vkDestroyDescriptorSetLayout(engine.device, camera_layout, nullptr);
    vkDestroyDescriptorSetLayout(engine.device, transform_layout, nullptr);
    vkDestroyDescriptorSetLayout(engine.device, material_layout, nullptr);
    vkDestroyDescriptorSetLayout(engine.device, light_layout, nullptr);
    vkDestroyDescriptorSetLayout(engine.device, attachment_layout, nullptr);
    
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    */

    // vkDestroyCommandPool(device, graphics.pool, nullptr);
    // vkDestroyCommandPool(device, compute.pool, nullptr);
    // vkDestroyCommandPool(device, transfer.pool, nullptr);
    // vkDestroyCommandPool(device, async.pool, nullptr);
    
    vkDestroyDevice(device, nullptr);

    vkDestroyInstance(instance, nullptr);

    glfwTerminate();
}

/*
uint32_t Engine::memory_type_index(VkMemoryRequirements& requirements, VkMemoryPropertyFlags memory_property_flags)
{
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
*/

/*
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
*/