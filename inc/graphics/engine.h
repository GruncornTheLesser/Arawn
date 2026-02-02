#pragma once
#include <graphics/vulkan.h>
#include <map>
#include <vector>

// device resources
namespace Arawn {
    enum QueueType : uint32_t { GRAPHICS, COMPUTE, ASYNC, TRANSFER, PRESENT };
    
    // fwd declare graph for friend declaration
    namespace Render { class Graph; }

    struct DescriptorLayoutInfo;

    class Engine {
    public:
        Engine();
        ~Engine();
        Engine(Engine&&) = delete;
        Engine(const Engine&) = delete;
        Engine& operator=(Engine&&) = delete;
        Engine& operator=(const Engine&) = delete;

        VK_TYPE(VkDescriptorSetLayout) setLayout(const std::vector<VK_TYPE(VkDescriptorSetLayoutBinding)>& bindings);

        VK_TYPE(VkInstance) instance;   // vulkan instance
        VK_TYPE(VkPhysicalDevice) gpu;  // selected gpu
        VK_TYPE(VkDevice) device;       // logical device
        
        uint32_t family[5];
        VK_TYPE(VkQueue) queue[5];

        VK_TYPE(VmaAllocator) allocator;
        
        std::map<DescriptorLayoutInfo, VK_TYPE(VkDescriptorSetLayout)> setLayouts;
    };

	extern Engine engine;
}