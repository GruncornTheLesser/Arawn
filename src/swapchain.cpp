#define ARAWN_IMPLEMENTATION
#include "swapchain.h"
#include "engine.h"
#include <algorithm>
#include "window.h"
#include <ranges>
#include <algorithm>


Swapchain::Swapchain()
{
    // create surface
    VK_ASSERT(glfwCreateWindowSurface(engine.instance, window.window, nullptr, &surface));
    swapchain = nullptr;
}

Swapchain::~Swapchain() {
    uint32_t image_count;
    vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr);
    
    vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
 
    vkDestroySurfaceKHR(engine.instance, surface, nullptr);
}

void Swapchain::recreate() {
    while (window.minimized()) { glfwWaitEvents(); }
    
    VkSwapchainKHR old_swapchain = swapchain;

    VkSurfaceCapabilitiesKHR capabilities;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine.gpu, surface, &capabilities));
    { // get swapchain extent
        if (capabilities.currentExtent.width == 0xffffffff) {
            extent = window.get_resolution();
        } 
        else {
            extent = glm::clamp(
                glm::uvec2(capabilities.currentExtent.width, capabilities.currentExtent.height), 
                glm::uvec2(capabilities.minImageExtent.width, capabilities.minImageExtent.height), 
                glm::uvec2(capabilities.maxImageExtent.width, capabilities.maxImageExtent.height));
        }
    }

    uint32_t image_count;
    { // get image count
        image_count = settings.frame_count;
        image_count = std::max<uint32_t>(image_count, capabilities.minImageCount);
        
        if (capabilities.maxImageCount != 0) {
            image_count = std::min<uint32_t>(image_count, capabilities.maxImageCount);
        }

    }
    
    { // get present mode
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
        std::vector<VkPresentModeKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));

        if (settings.low_latency_enabled()) {
            bool frame_skip_support;
            if (settings.vsync_enabled()) {
                frame_skip_support = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_MAILBOX_KHR) != supported.end();
            } else {
                frame_skip_support = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end();
            } 
            if (!frame_skip_support) {
                settings.set_low_latency(LowLatency::DISABLED);
            }
        }

        if (settings.vsync_enabled()) {
            present_mode = settings.low_latency_enabled() ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
        } else {
            present_mode = settings.low_latency_enabled() ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    { // get surface colour format
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, supported.data()));

        for (VkSurfaceFormatKHR& surface_format : supported) {
            switch (surface_format.format) {
                case(VK_FORMAT_R8G8B8A8_UNORM): break;
                case(VK_FORMAT_B8G8R8A8_UNORM): break;
                default: continue;
            }
            format = surface_format.format;
            colour_space = surface_format.colorSpace;
        }
    }

    { // init swapchain
        std::array<uint32_t, 2> queue_families = { engine.present.family, engine.graphics.family };
        auto unique_queue_families = std::ranges::unique(queue_families);

        VkSwapchainCreateInfoKHR info{
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, nullptr, 0, 
            surface, image_count, format, colour_space, { extent.x, extent.y }, 1,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
            unique_queue_families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE, 
            static_cast<uint32_t>(unique_queue_families.size()), unique_queue_families.data(), 
            VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,  // can rotate screen etc, normally for mobile
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,      // blend with other windows or not
            present_mode, 
            VK_TRUE,                                // skip draw to obscured pixels
            old_swapchain
        };

        VK_ASSERT(vkCreateSwapchainKHR(engine.device, &info, nullptr, &swapchain));
    }

    if (old_swapchain != nullptr) { // destroy old swapchain
        VK_ASSERT(vkDeviceWaitIdle(engine.device));
           
        uint32_t old_image_count;
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, old_swapchain, &old_image_count, nullptr));
        
        vkDestroySwapchainKHR(engine.device, old_swapchain, nullptr);
    }
    
    vkDeviceWaitIdle(engine.device);
}

