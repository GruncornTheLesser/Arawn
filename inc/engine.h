#pragma once
#include "vulkan.h"
#include <span>

class Engine {    
public:
    Engine();
    ~Engine();
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    VK_ENUM(VkFormat) select_buffer_format(std::span<VK_ENUM(VkFormat)> formats, VK_ENUM(VkFormatFeatureFlags) features) const;
    VK_ENUM(VkFormat) select_image_format(std::span<VK_ENUM(VkFormat)> formats, VK_ENUM(VkFormatFeatureFlags) features) const;
    uint32_t get_memory_index(uint32_t type_bits, VK_ENUM(VkMemoryPropertyFlags) flags) const;

    uint32_t family_set_data[3];                // array of unique queue families used
    uint32_t family_set_count;                  // length of family sets array
    
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
    struct {
        struct {
            VK_ENUM(VkFormat) depth, rgba, rgb, rg, r;
        } attachment;
    } format;
};

extern Engine engine;