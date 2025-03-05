#pragma once
#include "vulkan.h"
#include "settings.h"
#include "dispatcher.h"
#include "input.h"

#include <vector>


class Window : public Dispatcher<Key::Event, Mouse::Event> 
{
    friend class Swapchain;
public:
    Window();
    ~Window();
    
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;
    
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
    void set_title(const char* title);
    
    void set_resolution(glm::uvec2 res);
    auto get_resolution() const -> glm::uvec2;
    auto enum_resolutions(float ratio = 4.0f / 3.0f, float precision=0.1f) const -> std::vector<glm::uvec2>;

    void set_display_mode(DisplayMode mode);
    auto get_display_mode() const -> DisplayMode;
    auto enum_display_modes() const -> std::vector<DisplayMode>;

    // attributes
    auto closed() const -> bool;
    auto minimized() const -> bool;

private:
    VK_TYPE(GLFWwindow*) window = nullptr;
};

extern Window window;