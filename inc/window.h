#pragma once
#include "vulkan.h"
#include <utility>
#include <vector>
#include "dispatcher.h"

enum class DisplayMode { WINDOWED, FULLSCREEN }; // EXCLUSIVE

struct Key {
    enum Code : uint8_t {
        ESC=0, F1=1, F2=2, F3=3, F4=4, F5=5, F6=6, F7=7, F8=8, F9=9, F10=10, F11=11, F12=12, 
        PRINT_SCREEN=13, DEL=14, TAB=15, ENTER=16, BACKSPACE=17, INSERT=18, HOME=21, END=22, 
        L_SUPER=23, R_SUPER=24, L_SHIFT=25, R_SHIFT=26, L_CTRL=27, R_CTRL=28, L_ALT=29, R_ALT=30, MENU=31, 
        SPACE=32, ARROW_RIGHT=33, ARROW_LEFT=34, ARROW_DOWN=35, ARROW_UP=36, PAGE_UP=37, PAGE_DOWN=38, 
        APOSTROPHE='\'',    // 39
        COMMA=',',          // 44
        MINUS='-',          // 45
        PERIOD='.',         // 46
        SLASH='/',          // 47
        NUM_0='0', NUM_1='1', NUM_2='2', NUM_3='3', NUM_4='4', // 48 - 57
        NUM_5='5', NUM_6='6', NUM_7='7', NUM_8='8', NUM_9='9',
        SEMICOLON=';',      // 59
        EQUAL='=',          // 61
        CAPS_LOCK=62, SCROLL_LOCK=63, NUM_LOCK=64,
        A='A', B='B', C='C', D='D', E='E', F='F', G='G', H='H', I='I', J='J', K='K', L='L', M='M', // 65 - 90
        N='N', O='O', P='P', Q='Q', R='R', S='S', T='T', U='U', V='V', W='W', X='X', Y='Y', Z='Z', 
        LEFT_BRACKET='[',   // 91
        BACKSLASH='\\',     // 92
        RIGHT_BRACKET=']',  // 93
        GRAVE_ACCENT='`',   // 96
    };
    enum State { UP=0, DOWN=128 };
    struct Event {
        Event(uint32_t flags) : flags(flags) { }
    
        bool operator==(uint8_t val) { return val == flags; }
        bool operator==(Code val) const { return (code_mask & flags) == val; }
        bool operator==(State val) const { return (state_mask & flags) == val; }
        
        Code code() { return static_cast<Code>(flags & code_mask); }
        State state() { return static_cast<State>(flags & state_mask); }
        const uint8_t flags;
    private:
        static const uint8_t code_mask =  0b01111111;
        static const uint8_t state_mask = 0b10000000;
    };
};

struct Mouse {
    enum Code : uint8_t { LEFT=0, RIGHT=1, MIDDLE=2, SCROLL=8 };
    enum State : uint8_t { UP=0, DOWN = 16 };
    
    struct Event { 
        Event(uint32_t flags, int x, int y) : flags(flags), x(x), y(y) { }

        Code code() { return static_cast<Code>(flags & code_mask); }
        State state() { return static_cast<State>(flags & state_mask); }

        const int32_t x, y;
        const uint8_t flags;
    private:
        static const uint8_t code_mask =  0b00001111;
        static const uint8_t state_mask = 0b00010000;
    };
};

struct ResizeEvent { uint32_t width, height; };

class Window : public Dispatcher<Key::Event, Mouse::Event> 
{
    friend class Renderer;
public:
    struct Settings { struct { uint32_t x, y; } resolution; DisplayMode display_mode; };
    Window(uint32_t width = 640, uint32_t height = 400, const char* title ="", DisplayMode display = DisplayMode::WINDOWED);
    ~Window();
    
    Window(Window&&);
    Window& operator=(Window&&);
    
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    
    void set_title(const char* title);
    void get_title() const;
    
    void set_resolution(uint32_t width, uint32_t height);
    auto get_resolution() const -> std::pair<uint32_t, uint32_t>;
    auto enum_resolutions() const -> std::vector<std::pair<uint32_t, uint32_t>>;

    void set_display_mode(DisplayMode mode);
    auto get_display_mode() -> DisplayMode;
    auto enum_display_modes() -> std::vector<DisplayMode>;

    // attributes
    auto closed() const -> bool;
    auto minimized() const -> bool;

private:
    static void key_callback(VK_TYPE(GLFWwindow*) glfwWindow, int key, int scancode, int action, int mods);
    static void char_callback(VK_TYPE(GLFWwindow*) glfwWindow, unsigned int codepoint);
    static void mouse_move_callback(VK_TYPE(GLFWwindow*) glfwWindow, double xpos, double ypos);
    static void mouse_scroll_callback(VK_TYPE(GLFWwindow*) glfwWindow, double xoffset, double yoffset);
    static void mouse_button_callback(VK_TYPE(GLFWwindow*) glfwWindow, int button, int action, int mods);
    static void window_resize_callback(VK_TYPE(GLFWwindow*) glfwWindow, int width, int height);

    VK_TYPE(GLFWmonitor*) monitor = nullptr;
    VK_TYPE(GLFWwindow*) window = nullptr;

};


