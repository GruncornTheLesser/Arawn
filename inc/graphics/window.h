#pragma once
#include <graphics/vulkan.h>
#include <core/settings.h>
#include <chrono>

namespace Arawn {
    class Window {       
        friend class Swapchain;
        
        static void char_callback(GLFW_WINDOW window, unsigned int codepoint);
        static void key_callback(GLFW_WINDOW window, int key, int scancode, int action, int mods);
        static void mouse_move_callback(GLFW_WINDOW window, double xpos, double ypos);
        static void mouse_scroll_callback(GLFW_WINDOW window, double xoffset, double yoffset);
        static void mouse_button_callback(GLFW_WINDOW window, int button, int action, int mods);
    
        using time = std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds>;
        
        static GLFW_WINDOW createWindow(const char* name);
        static VK_TYPE(VkSurfaceKHR) createSurface(GLFW_WINDOW window);

    public:
        Window(const char* name);
        ~Window();
        
        Window(Window&&);
        Window& operator=(Window&&);

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        
        void resize(uint32_t x, uint32_t y);
        std::pair<uint32_t, uint32_t> size() const;
        std::vector<std::pair<uint32_t, uint32_t>> enumResolutions(float ratio, float precision=0.1f) const;
    
        void setDisplayMode(DisplayMode::Enum mode);
        DisplayMode::Enum getDisplayMode() const;
    
        void poll();

        void close();

        // attributes
        bool closed() const;
        bool minimized() const;
    private:
        time uptime;
        GLFW_WINDOW window;
    };
}
