#pragma once
#include <graphics/vulkan.h>

namespace Arawn {
    class Swapchain {
    public:
        Swapchain(VK_TYPE(VkSurfaceKHR) surface);

        ~Swapchain();
        Swapchain(Swapchain&&);
        Swapchain& operator=(Swapchain&&);
        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
    
        void recreate(uint32_t width, uint32_t height);
        
    private:        
        VK_ENUM(VkFormat) format;
        VK_ENUM(VkColorSpaceKHR) colour;
        VK_ENUM(VkPresentModeKHR) present;
    
        VK_TYPE(VkSurfaceKHR) surface;
        VK_TYPE(VkSwapchainKHR) swapchain;
    };
}