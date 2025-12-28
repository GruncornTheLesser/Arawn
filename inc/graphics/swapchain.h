#pragma once
#include "vulkan.h"
#include "settings.h"
#include <vector>

class Swapchain {
public:
    Swapchain();
    ~Swapchain();
    Swapchain(Swapchain&&);
    Swapchain& operator=(Swapchain&&);
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    void recreate();

    glm::uvec2 extent;
    
    VK_ENUM(VkFormat) format;
    VK_ENUM(VkColorSpaceKHR) colour_space;
    VK_ENUM(VkPresentModeKHR) present_mode;

    VK_TYPE(VkSurfaceKHR) surface;
    VK_TYPE(VkSwapchainKHR) swapchain;
};

extern Swapchain swapchain;