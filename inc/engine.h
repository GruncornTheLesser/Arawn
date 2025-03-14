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
    struct {
        uint32_t family;
        VK_TYPE(VkQueue) queue;
    } present;
    struct {
        uint32_t family;
        VK_TYPE(VkQueue) queue;
        VK_TYPE(VkCommandPool) pool;
    } graphics, compute;

    VK_TYPE(VkDescriptorPool) descriptor_pool;
    VK_TYPE(VkDescriptorSetLayout) object_layout;
    VK_TYPE(VkDescriptorSetLayout) camera_layout;
    VK_TYPE(VkDescriptorSetLayout) light_layout;
    VK_TYPE(VkDescriptorSetLayout) material_layout;
};

extern Engine engine;