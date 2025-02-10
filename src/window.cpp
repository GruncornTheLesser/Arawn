#define VK_IMPLEMENTATION
#include "window.h"
#include "engine.h"

Window::Window(uint32_t width, uint32_t height, DisplayMode display, VsyncMode vsync)
 : width(width), height(height), display_mode(display), vsync_mode(vsync), swapchain(VK_NULL_HANDLE)
{
    
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    window = glfwCreateWindow(width, height, "", nullptr, nullptr); 
    assert(window != nullptr);
    
    glfwCreateWindowSurface(engine.instance, window, nullptr, &surface);
    
    create_swapchain();
}


void Window::create_swapchain() {
    VkSwapchainKHR old_swapchain = swapchain;

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine.gpu, surface, &capabilities);
    
    { // check vsync mode support
        uint32_t count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr);

        std::vector<VkPresentModeKHR> supported(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data());

        if (std::find(supported.begin(), supported.end(), static_cast<VkPresentModeKHR>(vsync_mode)) == supported.end())
            vsync_mode = VsyncMode::IMMEDIATE;  // default garanteed FIFO
    }
        
    VkFormat format = VK_FORMAT_UNDEFINED;
    { // get format
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, nullptr);

        std::vector<VkSurfaceFormatKHR> surface_formats(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, surface_formats.data());
        
        for (auto& surface_data : surface_formats) {
            if (surface_data.format == VK_FORMAT_R8G8B8A8_SRGB || surface_data.format == VK_FORMAT_B8G8R8A8_SRGB || 
                surface_data.format == VK_FORMAT_R8G8B8A8_UNORM || surface_data.format == VK_FORMAT_B8G8R8A8_UNORM) {
                format = surface_data.format;
                break;
            }
        }
        assert(format != VK_FORMAT_UNDEFINED);
    }

    // get unique queues
    uint32_t family_set[3];
    uint32_t family_set_count = engine.get_queue_family_set(family_set);
    
    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface;
    info.imageExtent = { width, height };
    info.imageFormat = format;
    info.presentMode = static_cast<VkPresentModeKHR>(vsync_mode);
    info.imageSharingMode = family_set_count == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
    info.pQueueFamilyIndices = family_set;
    info.queueFamilyIndexCount = family_set_count;
    if (capabilities.maxImageCount == 0) info.minImageCount = MAX_IMAGE_COUNT;
    else info.minImageCount = std::min(MAX_IMAGE_COUNT, capabilities.maxImageCount);

    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;      // depends on purpose of swapchain, might want VK_IMAGE_USAGE_TRANSFER_DST_BIT for deferred renderer
    info.imageArrayLayers = 1;                                  // 1 unless VR
    info.preTransform = capabilities.currentTransform;          // can rotate screen etc
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;    // blend with other windows or not
    info.clipped = VK_TRUE;                                     // obscured pixels dont need to be written to
    info.oldSwapchain = old_swapchain;
    
    VK_ASSERT(vkCreateSwapchainKHR(engine.device, &info,nullptr, &swapchain));

    if (old_swapchain != VK_NULL_HANDLE) {
        for (uint32_t i = 0; i < get_image_count(); ++i)
            vkDestroyImageView(engine.device, image_views[i], nullptr);

        vkDestroySwapchainKHR(engine.device, old_swapchain, nullptr);
    }

    VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &info.minImageCount, images));

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    for (uint32_t i = 0; i < info.minImageCount; ++i) {
        view_info.image = images[i];
        vkCreateImageView(engine.device, &view_info, nullptr, &image_views[i]);
    }
}

Window::~Window() {
    if (window != nullptr) {        
        for (uint32_t i = 0; i < get_image_count(); ++i )
            vkDestroyImageView(engine.device, image_views[i], nullptr);
        vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
        
        vkDestroySurfaceKHR(engine.instance, surface, nullptr);
        glfwDestroyWindow(window);
    }
}

Window::Window(Window&& other) {
    width = other.width; 
    height = other.height;
    display_mode = other.display_mode;
    vsync_mode = other.vsync_mode;
    
    surface = other.surface;
    swapchain = other.swapchain;
    uint32_t image_count = get_image_count();
    std::copy(other.images, other.images + image_count, images);
    std::copy(other.image_views, other.image_views + image_count, image_views);
    window = other.window;

    other.window = nullptr;
}

Window& Window::operator=(Window&& other) {
    if (this != &other) { // if not assigned to self
        uint32_t image_count = get_image_count();

        if (window != nullptr) {
            for (uint32_t i = 0; i < image_count; ++i )
                vkDestroyImageView(engine.device, image_views[i], nullptr);
            vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
            vkDestroySurfaceKHR(engine.instance, surface, nullptr);
            glfwDestroyWindow(window);
        }

        if (other.window != nullptr) {
            width = other.width; 
            height = other.height;
            display_mode = other.display_mode;
            vsync_mode = other.vsync_mode;
            surface = other.surface;
            swapchain = other.swapchain;
            std::copy(other.images, other.images + image_count, images);
            std::copy(other.image_views, other.image_views + image_count, image_views);
            window = other.window;
        }
        
        other.window = nullptr;
    }
    return *this;
}

// modifiers
void Window::resize(uint32_t w, uint32_t h) { 
    // recreate swapchain
    vkDeviceWaitIdle(engine.device);
    width = w; height = h;
    create_swapchain();
}

void Window::set_vsync_mode(VsyncMode mode) { 
    vkDeviceWaitIdle(engine.device);

    vsync_mode = mode;
    
    create_swapchain();
}

void Window::set_display_mode(DisplayMode mode) { 
    switch (mode) {
        case DisplayMode::WINDOWED:
            glfwSetWindowMonitor(window, nullptr, 0, 0, width, height, 0);
            break;
        case DisplayMode::FULLSCREEN:
            break;
        case DisplayMode::EXCLUSIVE:
            break;
    }
    display_mode = mode;

    vkDeviceWaitIdle(engine.device);

    create_swapchain();
}

bool Window::closed() const {
    glfwPollEvents(); // TODO: this is for all windows but will do for now 
    return glfwWindowShouldClose(window);
}

// attributes
VsyncMode Window::get_vsync_mode() const {
    return vsync_mode;
}

DisplayMode Window::get_display_mode() const { 
    return display_mode; 
}

SurfaceFormat Window::get_format() const {
    return SurfaceFormat::BGRA_SRGB;
}

uint32_t Window::get_image_count() const {
    uint32_t count;
    VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &count, nullptr));
    return count;
}

std::vector<DisplayMode> Window::enum_display_modes() const {
    // TODO: check support for EXCLUSIVE FULLSCREEN ???
    return { DisplayMode::WINDOWED, DisplayMode::FULLSCREEN, DisplayMode::EXCLUSIVE };
}

std::vector<VsyncMode> Window::enum_vsync_mode() const {
    uint32_t count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr);

    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, modes.data());

    std::vector<VsyncMode> vsyncs(count);
    std::transform(modes.begin(), modes.end(), vsyncs.begin(), [](VkPresentModeKHR mode){ return static_cast<VsyncMode>(mode); });
    
    return vsyncs;
}