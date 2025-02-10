#pragma once
#include "vulkan.h"
#include <vector>

enum class DisplayMode { WINDOWED, FULLSCREEN, EXCLUSIVE };

enum class VsyncMode {
    ENUM_ENTRY(IMMEDIATE, VK_PRESENT_MODE_IMMEDIATE_KHR),
    ENUM_ENTRY(LOW_LATENCY_IMMEDIATE, VK_PRESENT_MODE_MAILBOX_KHR),
    ENUM_ENTRY(VSYNC, VK_PRESENT_MODE_FIFO_KHR),
    ENUM_ENTRY(LOW_LATENCY_VSYNC, VK_PRESENT_MODE_FIFO_RELAXED_KHR),
    ENUM_ENTRY(ADAPTIVE, VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR),
};

enum class SurfaceFormat {
    ENUM_ENTRY(RGBA_SRGB, VK_FORMAT_R8G8B8A8_SRGB),
    ENUM_ENTRY(BGRA_SRGB, VK_FORMAT_B8G8R8A8_SRGB),
    ENUM_ENTRY(RGBA_UNORM, VK_FORMAT_R8G8B8A8_UNORM),
    ENUM_ENTRY(BGRA_UNORM, VK_FORMAT_B8G8R8A8_UNORM),
};

class Window {
public:
    Window(uint32_t width=600, uint32_t height=480, DisplayMode display=DisplayMode::WINDOWED, VsyncMode present=VsyncMode::VSYNC);
    ~Window();
    
    Window(Window&&);
    Window& operator=(Window&&);
    
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
    // modifiers
    void resize(uint32_t width, uint32_t height);
    void set_display_mode(DisplayMode mode); // shouldnt recreate the swapchain??? might tho...
    void set_vsync_mode(VsyncMode mode);

    bool closed() const;
    VsyncMode get_vsync_mode() const;
    DisplayMode get_display_mode() const;
    
    SurfaceFormat get_format() const;
    uint32_t get_image_count() const;

    std::vector<DisplayMode> enum_display_modes() const;
    std::vector<VsyncMode> enum_vsync_mode() const;
private:
    static const inline uint32_t MAX_IMAGE_COUNT = 4;
    void create_swapchain();
    
    uint32_t width, height;
    DisplayMode display_mode;
    VsyncMode vsync_mode;
    
    VK_TYPE(GLFWwindow*) window = nullptr;
    VK_TYPE(VkSurfaceKHR) surface;
    VK_TYPE(VkSwapchainKHR) swapchain;
    VK_TYPE(VkImage) images[MAX_IMAGE_COUNT];
    VK_TYPE(VkImageView) image_views[MAX_IMAGE_COUNT];
};


