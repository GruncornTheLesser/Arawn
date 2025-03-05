#define VK_IMPLEMENTATION
#include "swapchain.h"
#include "engine.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include "window.h"


Swapchain::Swapchain()
{
    // create surface
    VK_ASSERT(glfwCreateWindowSurface(engine.instance, window.window, nullptr, &surface));

    swapchain = nullptr;
    recreate_swapchain();

}

Swapchain::~Swapchain() {
    vkDeviceWaitIdle(engine.device);

    vkFreeCommandBuffers(engine.device, engine.graphics.pool, frame_count, cmd_buffers);
    for (uint32_t i = 0; i < frame_count; ++i) {
        vkDestroySemaphore(engine.device, render_finished_semaphore[i], nullptr);
        vkDestroySemaphore(engine.device, image_available_semaphore[i], nullptr);
        vkDestroyFence(engine.device, in_flight_fence[i], nullptr);
    }

    uint32_t image_count;
    vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr);

    for (uint32_t i = 0; i < image_count; ++i) 
        vkDestroyImageView(engine.device, swapchain_views[i], nullptr);
    std::allocator<VkImageView>().deallocate(swapchain_views, image_count);
    
    vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
 
    vkDestroySurfaceKHR(engine.instance, surface, nullptr);
}

void Swapchain::set_vsync_mode(VsyncMode mode) { 
    settings.vsync_mode = mode;
    recreate_swapchain();
}

std::vector<VsyncMode> Swapchain::enum_vsync_modes() const {
    std::vector<VsyncMode> vsync_modes;

    vsync_modes.push_back(VsyncMode::ON); // guaranteed to be supported VK_PRESENT_MODE_FIFO_KHR

    uint32_t count;
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
    std::vector<VkPresentModeKHR> supported(count);
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));

    if (std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != supported.end() || 
        std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end())
    {
        vsync_modes.push_back(VsyncMode::OFF);
    }

    return vsync_modes;   
}

void Swapchain::set_sync_mode(SyncMode mode) {
    frame_count = static_cast<uint32_t>(mode);
    recreate_swapchain();
}

std::vector<SyncMode> Swapchain::enum_sync_modes() const {
    std::vector<SyncMode> sync_modes;
    sync_modes.push_back(SyncMode::DOUBLE);
    
    uint32_t count;
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
    std::vector<VkPresentModeKHR> supported(count);
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));
    
    return sync_modes;
}

void Swapchain::set_anti_alias(AntiAlias mode) {
    settings.anti_alias = mode;
    recreate_swapchain();
}

std::vector<AntiAlias> Swapchain::enum_anti_alias() const {
    std::vector<AntiAlias> anti_alias_modes = { AntiAlias::NONE }; // AntiAlias::FXAA_2, AntiAlias::FXAA_4, AntiAlias::FXAA_8, AntiAlias::FXAA_16
    
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(engine.gpu, &properties);

    if (properties.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_2_BIT)
        anti_alias_modes.push_back(AntiAlias::MSAA_2);

    if (properties.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_4_BIT)
        anti_alias_modes.push_back(AntiAlias::MSAA_4);

    if (properties.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_8_BIT)
        anti_alias_modes.push_back(AntiAlias::MSAA_8);

    if (properties.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_16_BIT)
        anti_alias_modes.push_back(AntiAlias::MSAA_16);

    return anti_alias_modes;
}

void Swapchain::recreate_swapchain() {
    while (window.minimized()) { glfwWaitEvents(); }
    
    VkSwapchainKHR old_swapchain = swapchain;   
    VkPresentModeKHR present_mode;
    VkFormat colour_format;
    VkColorSpaceKHR colour_space;
    VkFormat depth_format = engine.format.attachment.depth;
    VkSampleCountFlagBits sample_count = static_cast<VkSampleCountFlagBits>(settings.anti_alias);
    bool msaa_enabled = sample_count != VK_SAMPLE_COUNT_1_BIT;
    uint32_t attachment_count = msaa_enabled ? 3 : 2;

    VkSurfaceCapabilitiesKHR capabilities;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine.gpu, surface, &capabilities));
    { // get swapchain extent
        if (capabilities.currentExtent.width == 0xffffffff) {
            settings.resolution = window.get_resolution();
        } 
        else {
            settings.resolution = glm::clamp(
                glm::uvec2(capabilities.currentExtent.width, capabilities.currentExtent.height), 
                glm::uvec2(capabilities.minImageExtent.width, capabilities.minImageExtent.height), 
                glm::uvec2(capabilities.maxImageExtent.width, capabilities.maxImageExtent.height));
        }
    }
    
    if (capabilities.minImageCount > frame_count) {
        VkCommandBufferAllocateInfo cmd_buffer_info{ 
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 
            nullptr, 
            engine.graphics.pool, 
            VK_COMMAND_BUFFER_LEVEL_PRIMARY, 
            capabilities.minImageCount - frame_count // create additional
        };

        VK_ASSERT(vkAllocateCommandBuffers(engine.device, &cmd_buffer_info, cmd_buffers + frame_count));
        
        VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
        VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
        for (uint32_t i = frame_count; i < capabilities.minImageCount; ++i) {
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore_info, nullptr, &render_finished_semaphore[i]));
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore_info, nullptr, &image_available_semaphore[i]));
            VK_ASSERT(vkCreateFence(engine.device, &fence_info, nullptr, &in_flight_fence[i]));
        }
        frame_count = capabilities.minImageCount;
    }

    { // get present mode
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
        std::vector<VkPresentModeKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));

        bool frame_skip_support; // frame skip allows the swapchain to present the most recently created image
        switch (settings.vsync_mode) {
        case VsyncMode::OFF:
            frame_skip_support = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end();
            present_mode = frame_skip_support ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        case VsyncMode::ON:
            frame_skip_support = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_MAILBOX_KHR) != supported.end();
            present_mode = frame_skip_support ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
            break;
        }
    }

    { // get surface colour format
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, supported.data()));

        for (VkSurfaceFormatKHR& surface_format : supported) {
            switch (surface_format.format) {
                case(VK_FORMAT_R8G8B8A8_SRGB): break;
                case(VK_FORMAT_B8G8R8A8_SRGB): break;
                case(VK_FORMAT_R8G8B8A8_UNORM): break;
                case(VK_FORMAT_B8G8R8A8_UNORM): break;
                default: continue;
            }
            colour_format = surface_format.format;
            colour_space = surface_format.colorSpace;
        }
    }

    { // init swapchain
        VkSwapchainCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.pNext = nullptr;
        info.flags = 0;
        info.surface = surface;
        info.minImageCount = frame_count;
        info.imageFormat = colour_format;
        info.imageColorSpace = colour_space;
        info.imageExtent = { settings.resolution.x, settings.resolution.y };
        info.imageArrayLayers = 1;                                  // 1 unless VR
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;      // present is copy operation from render image
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;          // images only accessed by present queue
        info.queueFamilyIndexCount = 1;
        info.pQueueFamilyIndices = &engine.present.family;
        info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;  // can rotate screen etc, normally for mobile
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;    // blend with other windows or not
        info.presentMode = present_mode;
        info.clipped = VK_TRUE;                                     // skip draw to obscured pixels
        info.oldSwapchain = old_swapchain;

        VK_ASSERT(vkCreateSwapchainKHR(engine.device, &info, nullptr, &swapchain));
    }

    if (old_swapchain != nullptr) { // destroy old swapchain
        VK_ASSERT(vkDeviceWaitIdle(engine.device));
           
        uint32_t image_count;
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, old_swapchain, &image_count, nullptr));

        for (uint32_t i = 0; i < image_count; ++i) 
            vkDestroyImageView(engine.device, swapchain_views[i], nullptr);
        std::allocator<VkImageView>().deallocate(swapchain_views, image_count);
        
        vkDestroySwapchainKHR(engine.device, old_swapchain, nullptr);
    }

    { // init swapchain views
        uint32_t image_count;
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr));
        std::vector<VkImage> images(image_count);
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, images.data()));
        
        swapchain_views = std::allocator<VkImageView>().allocate(image_count);

        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = colour_format;
        // swizzling -> not needed
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        // mip mapping -> not needed
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        
        for (uint32_t i = 0; i < images.size(); ++i) {
            info.image = images[i];
            VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &swapchain_views[i]));
        }
    }
    
    vkDeviceWaitIdle(engine.device);
}
