#pragma once
#include "vulkan.h"

class Engine {    
public:
    Engine(const char* app_name="", const char* engine_name="");
    ~Engine();
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    uint32_t family_set_data[3];                // array of unique queue families used
    uint32_t family_set_count;                  // length of family sets array
    
    VK_TYPE(VkInstance) instance;               // vulkan instance
    VK_TYPE(VkPhysicalDevice) gpu;              // gpu selected
    VK_TYPE(VkDevice) device;
    VK_TYPE(VkDescriptorPool) descriptor_pool;
    struct {
        uint32_t family;
        VK_TYPE(VkQueue) queue;
        VK_TYPE(VkCommandPool) pool;
    } graphics, compute, present;
};

extern const Engine engine;