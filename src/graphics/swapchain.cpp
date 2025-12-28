#define ARAWN_IMPLEMENTATION
#include <graphics/swapchain.h>
#include <graphics/engine.h>
#include <core/settings.h>
#include <algorithm>

using namespace Arawn;

Swapchain::Swapchain(VkSurfaceKHR surface) : surface(surface), swapchain(VK_NULL_HANDLE) { }

Swapchain::~Swapchain()
{
    if (swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
    }

    if (surface != VK_NULL_HANDLE) 
    {
        vkDestroySurfaceKHR(engine.instance, surface, nullptr);
    }
}


Arawn::Swapchain::Swapchain(Swapchain&& other)
{
    format = other.format;
    colour = other.colour;
    present = other.present;

    surface = other.surface;
    swapchain = other.swapchain;

    other.swapchain = VK_NULL_HANDLE;
    other.surface = VK_NULL_HANDLE;
}

Arawn::Swapchain& Arawn::Swapchain::operator=(Swapchain&& other)
{
    if (this == &other) 
    {
        return *this;
    }
    
    if (swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
    }

    if (surface != VK_NULL_HANDLE) 
    {
        vkDestroySurfaceKHR(engine.instance, surface, nullptr);
    }

    format = other.format;
    colour = other.colour;
    present = other.present;

    surface = other.surface;
    swapchain = other.swapchain;

    other.swapchain = VK_NULL_HANDLE;
    other.surface = VK_NULL_HANDLE;

    return *this;
}

void Swapchain::recreate(uint32_t width, uint32_t height)
{
    // get surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine.gpu, surface, &capabilities));
    
    { // get swapchain extent
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            width  = std::clamp(capabilities.currentExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
            height = std::clamp(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }
    }

    uint32_t imageCount;
    { // get image count
        imageCount = settings.get<FrameCount>() == FrameCount::TRIPLE_BUFFERED ? 3 : 2;
        imageCount = std::max<uint32_t>(imageCount, capabilities.minImageCount);
        
        if (capabilities.maxImageCount != 0)
        {
            imageCount = std::min<uint32_t>(imageCount, capabilities.maxImageCount);
        }

    }
    
    { // get present mode
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
        std::vector<VkPresentModeKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));

        if (settings.get<LowLatency>())
        {
            bool frameSkipSupport;
            if (settings.get<VsyncMode>())
            {
                frameSkipSupport = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_MAILBOX_KHR) != supported.end();
            }
            else
            {
                frameSkipSupport = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end();
            } 
            
             // disable low latency if 
            if (!frameSkipSupport)
            {
                settings.set<LowLatency>(LowLatency::DISABLED);
            }
        }

        if (settings.get<VsyncMode>())
        {
            present = settings.get<LowLatency>() ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
        } 
        else
        {
            present = settings.get<LowLatency>() ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    { // get surface colour format
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, supported.data()));

        for (VkSurfaceFormatKHR& surface_format : supported)
        {
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
        uint32_t queueFamilies[2]{ engine.family[PRESENT], engine.family[GRAPHICS] };
        bool sharedPresentGraphicsQueueFamily = engine.family[PRESENT] == engine.family[GRAPHICS];

        VkSwapchainCreateInfoKHR info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, 
            .pNext = nullptr,
            .surface = surface,
            .minImageCount = imageCount, 
            .imageFormat = format,
            .imageColorSpace = colour, 
            .imageExtent = { width, height }, 
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
            .imageSharingMode = sharedPresentGraphicsQueueFamily ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT, 
            .queueFamilyIndexCount = static_cast<uint32_t>(sharedPresentGraphicsQueueFamily ? 1 : 2), 
            .pQueueFamilyIndices = queueFamilies, 
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,  // can rotate screen etc, normally for mobile
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,      // blend with other windows or not
            .presentMode = present,
            .clipped = VK_TRUE, // skip draw to obscured pixels
            .oldSwapchain = oldSwapchain
        };

        VK_ASSERT(vkCreateSwapchainKHR(engine.device, &info, nullptr, &swapchain));
    }

    // destroy old swapchain
    if (oldSwapchain != nullptr)
    { 
        vkDestroySwapchainKHR(engine.device, oldSwapchain, nullptr);
    }
}