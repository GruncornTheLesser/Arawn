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
    
    void set_sync_mode(SyncMode mode);
    std::vector<SyncMode> enum_sync_modes() const;
    
    void set_anti_alias(AntiAlias mode);
    std::vector<AntiAlias> enum_anti_alias() const;
    
    void set_depth_mode(DepthMode value);
    bool get_depth_mode() const;

    void recreate_swapchain();
   
    VK_TYPE(VkSurfaceKHR) surface;
    VK_TYPE(VkSwapchainKHR) swapchain;
    VK_TYPE(VkImageView)* swapchain_views;  // array of image views   

    uint32_t frame_count = 0;
    uint32_t frame_index = 0; // current frame index
    VK_TYPE(VkCommandBuffer) cmd_buffers[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore)     render_finished_semaphore[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore)     image_available_semaphore[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkFence)         in_flight_fence[MAX_FRAMES_IN_FLIGHT];
};

extern Swapchain swapchain;