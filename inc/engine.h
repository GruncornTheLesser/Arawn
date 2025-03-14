#pragma once
#include "vulkan.h"

class Engine {    
public:
    Engine();
    ~Engine();
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    VK_TYPE(VkInstance) instance;               // vulkan instance
    VK_TYPE(VkPhysicalDevice) gpu;              // gpu selected
    VK_TYPE(VkDevice) device;
    VK_TYPE(VkDescriptorPool) descriptor_pool;
    struct {
        uint32_t family;
        VK_TYPE(VkQueue) queue;
    } present;
    struct {
        uint32_t family;
        VK_TYPE(VkQueue) queue;
        VK_TYPE(VkCommandPool) pool;
    } graphics, compute;
};

extern Engine engine;