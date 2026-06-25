#pragma once
#include <chrono>
#include <render/core/vulkan.h>
#include <render/core/glm.h>
#include <atomic>
#include <cfloat>


namespace Arawn {
    enum class DisplayMode : uint32_t {
        WINDOWED   = 0, 
        FULLSCREEN = 1, 
        EXCLUSIVE  = 2, 
    };

    class Window {
        struct Info {
            const char* title = "Arawn";
            uint32_t width = 800;
            uint32_t height = 600;
            bool triple_buffered : 1 = false;
            bool vsync : 1 = false;
            bool low_latency : 1 = false;
            DisplayMode mode : 2;
        } state;
        
        friend class Renderer;
        using time = std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds>;
    public:
        Window(const Info& info);
        void recreate(const Info& info);
        void recreate();
        
        ~Window();
        
        Window(Window&&);
        Window& operator=(Window&&);

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        
        static void poll();

        void refresh();

        bool closed() const;

        bool minimized() const;
    private:
        time uptime;
        GLFW_WINDOW window;
    };
}
