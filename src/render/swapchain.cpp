#include <vulkan/vulkan_core.h>
#define ARAWN_IMPLEMENTATION
#include <render/core/engine.h>
#include <render/swapchain.h>
#include <algorithm>

using namespace Arawn;


/*
Swapchain::Swapchain(Info&& info) : swapchain(VK_NULL_HANDLE) {
    glfwCreateWindowSurface(engine.instance, info.window.window, NULL, &surface);
    *this = std::move(info);
}

Swapchain& Swapchain::operator=(Info&& info) {
    uint32_t         width, height;
    VkPresentModeKHR present;
    VkFormat         format;
    VkColorSpaceKHR  colour;
    { // get window scale
        int w, h;
        glfwGetWindowSize(info.window.window, &w, &h);

        width = static_cast<uint32_t>(w);
        height = static_cast<uint32_t>(h);
    }

    VkSurfaceCapabilitiesKHR capabilities;
    { // get surface capabilities
        VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine.gpu, surface, &capabilities));
    }
    
    { // get swapchain extent
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            width  = std::clamp(capabilities.currentExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
            height = std::clamp(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }
    }

    uint32_t image_count = info.triple_buffered ? 3 : 2;
    { // get image count
        image_count = std::max<uint32_t>(image_count, capabilities.minImageCount);
        
        if (capabilities.maxImageCount != 0)
        {
            image_count = std::min<uint32_t>(image_count, capabilities.maxImageCount);
        }
    }
    
    { // get present mode
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
        
        std::vector<VkPresentModeKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));

        if (info.low_latency) {
            if (info.vsync) {
                info.low_latency &= std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_MAILBOX_KHR) != supported.end();
            }
            else {
                info.low_latency &= std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end();
            } 
        }

        if (info.vsync) {
            present = info.low_latency ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
        } 
        else {
            present = info.low_latency ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    { // get surface colour format
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, supported.data()));

        for (VkSurfaceFormatKHR& surface_format : supported) {
            switch (surface_format.format)
            {
                case(VK_FORMAT_R8G8B8A8_SRGB): break;
                case(VK_FORMAT_B8G8R8A8_SRGB): break;
                case(VK_FORMAT_R8G8B8A8_UNORM): break;
                case(VK_FORMAT_B8G8R8A8_UNORM): break;
                default: continue;
            }
            format = surface_format.format;
            colour = surface_format.colorSpace;
        }
    }

    // when recreating with oldSwapchain it allows the driver to reuse resources where applicable
    VkSwapchainKHR oldSwapchain = swapchain;
    
    { // init swapchain
        // swapchain images are used by the graphics and present queues
        uint32_t queueFamilies[2]{ engine.family[Queue::PRESENT], engine.family[Queue::GRAPHICS] };
        bool shared_present_graphic = engine.family[Queue::PRESENT] == engine.family[Queue::GRAPHICS];

        VkSwapchainCreateInfoKHR info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, 
            .pNext = nullptr,
            .surface = surface,
            .minImageCount = image_count, 
            .imageFormat = format,
            .imageColorSpace = colour, 
            .imageExtent = { width, height }, 
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
            .imageSharingMode = shared_present_graphic ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT, 
            .queueFamilyIndexCount = static_cast<uint32_t>(shared_present_graphic ? 1 : 2), 
            .pQueueFamilyIndices = queueFamilies, 
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = present,
            .clipped = VK_TRUE, // skip draw to obscured pixels
            .oldSwapchain = oldSwapchain
        };

        VK_ASSERT(vkCreateSwapchainKHR(engine.device, &info, nullptr, &swapchain));
    }

    // destroy old swapchain
    if (oldSwapchain != nullptr) { 
        vkDestroySwapchainKHR(engine.device, oldSwapchain, nullptr);
    }

    return *this;
}

Swapchain::~Swapchain()
{
    if (swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
    }

    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(engine.instance, surface, nullptr);
    }
}


Arawn::Swapchain::Swapchain(Swapchain&& other) {
    surface = other.surface;
    swapchain = other.swapchain;

    other.swapchain = VK_NULL_HANDLE;
    other.surface = VK_NULL_HANDLE;
}

Arawn::Swapchain& Arawn::Swapchain::operator=(Swapchain&& other) {
    if (this == &other) {
        return *this;
    }
    
    if (swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
    }

    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(engine.instance, surface, nullptr);
    }

    surface = other.surface;
    swapchain = other.swapchain;

    other.swapchain = VK_NULL_HANDLE;
    other.surface = VK_NULL_HANDLE;

    return *this;
}
    */