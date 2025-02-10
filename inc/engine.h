#pragma once
#include "vulkan.h"

class Engine {
    friend class Window;

public:
    Engine(const char* app_name="", const char* engine_name="");
    ~Engine();
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

private:
    // generates the set of families
    uint32_t get_queue_family_set(uint32_t* data) const; // TODO: replace with span

    VK_TYPE(VkInstance) instance;
    VK_TYPE(VkPhysicalDevice) gpu;
    VK_TYPE(VkDevice) device;
    uint32_t graphic_family;
    uint32_t compute_family;
    uint32_t present_family;
    VK_TYPE(VkQueue) graphic_queue;
    VK_TYPE(VkQueue) compute_queue;
    VK_TYPE(VkQueue) present_queue;
};

extern Engine engine; 