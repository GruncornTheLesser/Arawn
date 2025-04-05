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

    // attributes 
    void set_vsync_mode(VsyncMode mode);
    std::vector<VsyncMode> enum_vsync_modes() const;
        
    void set_anti_alias(AntiAlias mode);
    std::vector<AntiAlias> enum_anti_alias() const;

    void recreate();

    glm::uvec2 extent;
    uint32_t image_count;
    
    VK_ENUM(VkFormat) format;
    VK_ENUM(VkColorSpaceKHR) colour_space;
    VK_ENUM(VkPresentModeKHR) present_mode;

    VK_TYPE(VkSurfaceKHR) surface;
    VK_TYPE(VkSwapchainKHR) swapchain;
    std::vector<VK_TYPE(VkImageView)> view;
};

extern Swapchain swapchain;