#pragma once
#include "vulkan.h"
#include <utility>

enum class DisplayMode { WINDOWED, FULLSCREEN, EXCLUSIVE };

class Window {
    friend class Renderer;
public:
    struct Settings { struct { uint32_t x, y; } resolution; DisplayMode display_mode; };
    Window(uint32_t width, uint32_t height, DisplayMode display);
    
    Window(uint32_t width=600, uint32_t height=480);
    ~Window();
    
    Window(Window&&);
    Window& operator=(Window&&);
    
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
    // attributes
    auto closed() const -> bool;
    auto minimized() const -> bool;
    auto get_resolution() const -> std::pair<uint32_t, uint32_t>;

private:
    VK_TYPE(GLFWwindow*) window = nullptr;
};


