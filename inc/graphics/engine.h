#pragma once
#include <graphics/vulkan.h>

// device resources
namespace Arawn {
    enum QueueType : uint32_t { PRESENT, GRAPHICS, COMPUTE, ASYNC, TRANSFER };

    class Engine {
        friend class Window;
		friend class Swapchain;
    public:
        Engine();
        ~Engine();
        Engine(Engine&&) = delete;
        Engine(const Engine&) = delete;
        Engine& operator=(Engine&&) = delete;
        Engine& operator=(const Engine&) = delete;
        
    private:
        VK_TYPE(VkInstance) instance;   // vulkan instance
        VK_TYPE(VkPhysicalDevice) gpu;  // selected gpu
        VK_TYPE(VkDevice) device;       // logical device
        
        uint32_t family[5];
        VK_TYPE(VkQueue) queue[5];

        VK_TYPE(VmaAllocator) allocator;
    };

	extern Engine engine;
}