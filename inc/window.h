#pragma once
#include "vulkan.h"
#include "dispatcher.h"
#include <glm/glm.hpp>
#include <vector>
#include "input.h"

enum class DisplayMode { WINDOWED=0, FULLSCREEN=1, EXCLUSIVE=2 };

class Window : public Dispatcher<Key::Event, Mouse::Event> 
{
    friend class Renderer;
public:
    struct Settings { struct { uint32_t x, y; } resolution; DisplayMode display_mode; };
    Window(uint32_t width = 640, uint32_t height = 400, DisplayMode display = DisplayMode::WINDOWED);
    ~Window();
    
    Window(Window&&);
    Window& operator=(Window&&);
    
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
    void set_title(const char* title);
    
    void set_resolution(uint32_t width, uint32_t height);
    auto get_resolution() const -> glm::uvec2;
    auto enum_resolutions(float ratio, float precision=0.2f) const -> std::vector<glm::uvec2>;

    void set_display_mode(DisplayMode mode);
    auto get_display_mode() const -> DisplayMode;
    auto enum_display_modes() const -> std::vector<DisplayMode>;

    // attributes
    auto closed() const -> bool;
    auto minimized() const -> bool;

private:
    static void key_callback(VK_TYPE(GLFWwindow*) glfwWindow, int key, int scancode, int action, int mods);
    static void char_callback(VK_TYPE(GLFWwindow*) glfwWindow, unsigned int codepoint);
    static void mouse_move_callback(VK_TYPE(GLFWwindow*) glfwWindow, double xpos, double ypos);
    static void mouse_scroll_callback(VK_TYPE(GLFWwindow*) glfwWindow, double xoffset, double yoffset);
    static void mouse_button_callback(VK_TYPE(GLFWwindow*) glfwWindow, int button, int action, int mods);
protected:
    VK_TYPE(GLFWwindow*) window = nullptr;
};


