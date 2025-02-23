#define VK_IMPLEMENTATION
#include "window.h"
#include <algorithm>

Key::Code get_key_code(int key) {
    switch (key) {
        case GLFW_KEY_SPACE: return Key::SPACE; break;
        case GLFW_KEY_APOSTROPHE: return Key::APOSTROPHE; break;
        case GLFW_KEY_COMMA: return Key::COMMA; break;
        case GLFW_KEY_MINUS: return Key::MINUS; break;
        case GLFW_KEY_PERIOD: return Key::PERIOD; break;
        case GLFW_KEY_SLASH: return Key::SLASH; break;
        case GLFW_KEY_0: return Key::NUM_0; break;
        case GLFW_KEY_1: return Key::NUM_1; break;
        case GLFW_KEY_2: return Key::NUM_2; break;
        case GLFW_KEY_3: return Key::NUM_3; break;
        case GLFW_KEY_4: return Key::NUM_4; break;
        case GLFW_KEY_5: return Key::NUM_5; break;
        case GLFW_KEY_6: return Key::NUM_6; break;
        case GLFW_KEY_7: return Key::NUM_7; break;
        case GLFW_KEY_8: return Key::NUM_8; break;
        case GLFW_KEY_9: return Key::NUM_9; break;
        case GLFW_KEY_SEMICOLON: return Key::SEMICOLON; break;
        case GLFW_KEY_EQUAL: return Key::EQUAL; break;
        case GLFW_KEY_A: return Key::A; break;
        case GLFW_KEY_B: return Key::B; break;
        case GLFW_KEY_C: return Key::C; break;
        case GLFW_KEY_D: return Key::D; break;
        case GLFW_KEY_E: return Key::E; break;
        case GLFW_KEY_F: return Key::F; break;
        case GLFW_KEY_G: return Key::G; break;
        case GLFW_KEY_H: return Key::H; break;
        case GLFW_KEY_I: return Key::I; break;
        case GLFW_KEY_J: return Key::J; break;
        case GLFW_KEY_K: return Key::K; break;
        case GLFW_KEY_L: return Key::L; break;
        case GLFW_KEY_M: return Key::M; break;
        case GLFW_KEY_N: return Key::N; break;
        case GLFW_KEY_O: return Key::O; break;
        case GLFW_KEY_P: return Key::P; break;
        case GLFW_KEY_Q: return Key::Q; break;
        case GLFW_KEY_R: return Key::R; break;
        case GLFW_KEY_S: return Key::S; break;
        case GLFW_KEY_T: return Key::T; break;
        case GLFW_KEY_U: return Key::U; break;
        case GLFW_KEY_V: return Key::V; break;
        case GLFW_KEY_W: return Key::W; break;
        case GLFW_KEY_X: return Key::X; break;
        case GLFW_KEY_Y: return Key::Y; break;
        case GLFW_KEY_Z: return Key::Z; break;
        case GLFW_KEY_LEFT_BRACKET: return Key::LEFT_BRACKET; break;
        case GLFW_KEY_BACKSLASH: return Key::BACKSLASH; break;
        case GLFW_KEY_RIGHT_BRACKET: return Key::RIGHT_BRACKET; break;
        case GLFW_KEY_GRAVE_ACCENT: return Key::GRAVE_ACCENT; break;
        // case GLFW_KEY_WORLD_1: return Key::; break;
        // case GLFW_KEY_WORLD_2: return Key::; break;
        case GLFW_KEY_ESCAPE: return Key::ESC; break;
        case GLFW_KEY_ENTER: return Key::ENTER; break;
        case GLFW_KEY_TAB: return Key::TAB; break;
        case GLFW_KEY_BACKSPACE: return Key::BACKSPACE; break;
        case GLFW_KEY_INSERT: return Key::INSERT; break;
        case GLFW_KEY_DELETE: return Key::DEL; break;
        case GLFW_KEY_RIGHT: return Key::ARROW_RIGHT; break;
        case GLFW_KEY_LEFT: return Key::ARROW_LEFT; break;
        case GLFW_KEY_DOWN: return Key::ARROW_DOWN; break;
        case GLFW_KEY_UP: return Key::ARROW_UP; break;
        case GLFW_KEY_PAGE_UP: return Key::PAGE_UP; break;
        case GLFW_KEY_PAGE_DOWN: return Key::PAGE_DOWN; break;
        case GLFW_KEY_HOME: return Key::HOME; break;
        case GLFW_KEY_END: return Key::END; break;
        case GLFW_KEY_CAPS_LOCK: return Key::CAPS_LOCK; break;
        case GLFW_KEY_SCROLL_LOCK: return Key::SCROLL_LOCK; break;
        case GLFW_KEY_NUM_LOCK: return Key::NUM_LOCK; break;
        case GLFW_KEY_PRINT_SCREEN: return Key::PRINT_SCREEN; break;
        // case GLFW_KEY_PAUSE: return Key::; break;
        case GLFW_KEY_F1: return Key::F1; break;
        case GLFW_KEY_F2: return Key::F2; break;
        case GLFW_KEY_F3: return Key::F3; break;
        case GLFW_KEY_F4: return Key::F4; break;
        case GLFW_KEY_F5: return Key::F5; break;
        case GLFW_KEY_F6: return Key::F6; break;
        case GLFW_KEY_F7: return Key::F7; break;
        case GLFW_KEY_F8: return Key::F8; break;
        case GLFW_KEY_F9: return Key::F9; break;
        case GLFW_KEY_F10: return Key::F10; break;
        case GLFW_KEY_F11: return Key::F11; break;
        case GLFW_KEY_F12: return Key::F12; break;
        // case GLFW_KEY_F13: return Key::; break;
        // case GLFW_KEY_F14: return Key::; break;
        // case GLFW_KEY_F15: return Key::; break;
        // case GLFW_KEY_F16: return Key::; break;
        // case GLFW_KEY_F17: return Key::; break;
        // case GLFW_KEY_F18: return Key::; break;
        // case GLFW_KEY_F19: return Key::; break;
        // case GLFW_KEY_F20: return Key::; break;
        // case GLFW_KEY_F21: return Key::; break;
        // case GLFW_KEY_F22: return Key::; break;
        // case GLFW_KEY_F23: return Key::; break;
        // case GLFW_KEY_F24: return Key::; break;
        // case GLFW_KEY_F25: return Key::; break;
        // case GLFW_KEY_KP_0: return Key::; break;
        // case GLFW_KEY_KP_1: return Key::; break;
        // case GLFW_KEY_KP_2: return Key::; break;
        // case GLFW_KEY_KP_3: return Key::; break;
        // case GLFW_KEY_KP_4: return Key::; break;
        // case GLFW_KEY_KP_5: return Key::; break;
        // case GLFW_KEY_KP_6: return Key::; break;
        // case GLFW_KEY_KP_7: return Key::; break;
        // case GLFW_KEY_KP_8: return Key::; break;
        // case GLFW_KEY_KP_9: return Key::; break;
        // case GLFW_KEY_KP_DECIMAL: return Key::; break;
        // case GLFW_KEY_KP_DIVIDE: return Key::; break;
        // case GLFW_KEY_KP_MULTIPLY: return Key::; break;
        // case GLFW_KEY_KP_SUBTRACT: return Key::; break;
        // case GLFW_KEY_KP_ADD: return Key::; break;
        // case GLFW_KEY_KP_ENTER: return Key::; break;
        // case GLFW_KEY_KP_EQUAL: return Key::; break;
        case GLFW_KEY_LEFT_SHIFT: return Key::L_SHIFT; break;
        case GLFW_KEY_LEFT_CONTROL: return Key::L_CTRL; break;
        case GLFW_KEY_LEFT_ALT: return Key::L_ALT; break;
        case GLFW_KEY_LEFT_SUPER: return Key::L_SUPER; break;
        case GLFW_KEY_RIGHT_SHIFT: return Key::R_SHIFT; break;
        case GLFW_KEY_RIGHT_CONTROL: return Key::R_CTRL; break;
        case GLFW_KEY_RIGHT_ALT: return Key::R_ALT; break;
        case GLFW_KEY_RIGHT_SUPER: return Key::R_SUPER; break;
        case GLFW_KEY_MENU: return Key::MENU; break;
    }
}

const GLFWvidmode* select_vid_mode(GLFWmonitor* monitor, int width, int height) {
    int count;
    const GLFWvidmode* cur = glfwGetVideoModes(monitor, &count);
    const GLFWvidmode* end = cur + count;
    const GLFWvidmode* min = nullptr;
    // find min video mode greater than or equal to specified resolution
    do {
        if (cur->width < width) continue;
        if (cur->height < height) continue;
        min = cur;
        break;
    } while(++cur < end);
    do {
        if (cur->width < width) continue;
        if (cur->height < height) continue;
        if (cur->width > min->width) continue;
        if (cur->height > min->height) continue;
        min = cur;
    } while(++cur < end);
    return min;
}

Window::Window(uint32_t width, uint32_t height, DisplayMode display_mode)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vid_mode = select_vid_mode(monitor, width, height); // resolution must match monitor video mode
    
    switch (display_mode) {
    case DisplayMode::EXCLUSIVE: {
        window = glfwCreateWindow(vid_mode->width, vid_mode->height, "", monitor, nullptr);
        break;
    }
    case DisplayMode::FULLSCREEN: {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
        window = glfwCreateWindow(vid_mode->width, vid_mode->height, "", nullptr, nullptr);
        break;
    }
    case DisplayMode::WINDOWED: {
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
        window = glfwCreateWindow(vid_mode->width, vid_mode->height, "", nullptr, nullptr);
        break;
    }
    
    }
    
    glfwFocusWindow(window);
    GLFW_ASSERT(window);

    glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, key_callback);
	//glfwSetCharCallback(window, char_callback);
	glfwSetScrollCallback(window, mouse_scroll_callback);
	glfwSetCursorPosCallback(window, mouse_move_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
}

Window::~Window() {
    if (window != nullptr) 
        glfwDestroyWindow(window);
}

Window::Window(Window&& other) {
    window = other.window;
    other.window = nullptr;
    glfwSetWindowUserPointer(window, this);
}

Window& Window::operator=(Window&& other) {
    if (this != &other) { // if not assigned to self
        if (window != nullptr)
            glfwDestroyWindow(window);
        
        std::swap(window, other.window);
        glfwSetWindowUserPointer(window, this);
    }
    return *this;
}

void Window::set_title(const char* title) {
    glfwSetWindowTitle(window, title);
}

void Window::set_resolution(uint32_t width, uint32_t height) {
    glfwSetWindowSize(window, width, height);
}

auto Window::get_resolution() const -> glm::uvec2 {
    int x, y;
    glfwGetWindowSize(window, &x, &y);
    return { x, y };
}

auto Window::enum_resolutions(float ratio, float precision) const -> std::vector<glm::uvec2> {
    GLFWmonitor* monitor = glfwGetWindowMonitor(window);
    if (monitor == nullptr) monitor = glfwGetPrimaryMonitor();

    int count;
    const GLFWvidmode* video_modes = glfwGetVideoModes(monitor, &count);
    std::vector<glm::uvec2> resolutions(count);
    
    for (int i = 0; i < count; ++i)
        resolutions[i] = { video_modes[i].width, video_modes[i].height }; 
    
    std::vector<glm::uvec2>::iterator end = resolutions.end();
    end = std::remove_if(resolutions.begin(), end, [=](auto& res) { return std::abs(((float)res.x / res.y) - ratio) > precision; });
    std::sort(resolutions.begin(), end, [](auto& lhs, auto& rhs) { if (lhs.x != rhs.x) return lhs.x < rhs.x; else return lhs.y < rhs.y; });
    end = std::unique(resolutions.begin(), end);
    end = std::remove_if(resolutions.begin(), end, [](auto& res) { return res.x < 800 || res.y < 600; });
    resolutions.erase(end, resolutions.end());
    return resolutions;
}

void Window::set_display_mode(DisplayMode display_mode) {
    switch (display_mode) {
    case DisplayMode::EXCLUSIVE: {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        const GLFWvidmode* vid_mode = select_vid_mode(monitor, width, height); 
        glfwSetWindowMonitor(window, monitor, 0, 0, vid_mode->width, vid_mode->height, GLFW_DONT_CARE);
        break;
    }
    case DisplayMode::FULLSCREEN: {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glfwSetWindowMonitor(window, nullptr, 0, 0, width, height, GLFW_DONT_CARE);
        
        glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
        break;
    }
    case DisplayMode::WINDOWED: {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glfwSetWindowMonitor(window, nullptr, 50, 50, width, height, GLFW_DONT_CARE);
        glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_FALSE);
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
        break;
    }
    }
    glfwFocusWindow(window);
    glfwShowWindow(window); // hoping to get attention back
}
auto Window::get_display_mode() const -> DisplayMode {
    // checks features of 
    if (glfwGetWindowMonitor(window) != nullptr) 
        return DisplayMode::EXCLUSIVE;
    if (glfwGetWindowAttrib(window, GLFW_FLOATING) == GLFW_TRUE)
        return DisplayMode::FULLSCREEN;
    //if (glfwGetWindowAttrib(window, GLFW_DECORATED) == GLFW_TRUE) 
    return DisplayMode::WINDOWED;
}
auto Window::enum_display_modes() const -> std::vector<DisplayMode> {
    return { DisplayMode::WINDOWED, DisplayMode::FULLSCREEN, DisplayMode::EXCLUSIVE };
}

auto Window::closed() const -> bool {
    glfwPollEvents(); // TODO: this is for all windows but will do for now 
    return glfwWindowShouldClose(window);
}

auto Window::minimized() const -> bool {
    int x, y;
    glfwGetWindowSize(window, &x, &y);
    return x == 0 || y == 0;
}



void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) { 
	auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    wnd.template on<Key::Event>().invoke({
        static_cast<uint32_t>(get_key_code(key)) | static_cast<uint32_t>(action == GLFW_PRESS ? Key::UP : Key::DOWN) 
    });
}

void Window::mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { 
	double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
	glfwGetCursorPos(window, &x, &y);
    wnd.template on<Mouse::Event>().invoke({ 
        static_cast<uint32_t>(Mouse::SCROLL) | static_cast<uint32_t>(yoffset > 0 ? Mouse::UP : Mouse::DOWN), 
        static_cast<int>(x), 
        static_cast<int>(y) 
    });
}

void Window::mouse_move_callback(GLFWwindow* window, double xpos, double ypos) { 
	double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
	glfwGetCursorPos(window, &x, &y);
    wnd.template on<Mouse::Event>().invoke({ 
        0, 
        static_cast<int>(x), 
        static_cast<int>(y)
    });
}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) { 
	double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
	glfwGetCursorPos(window, &x, &y);
    
    wnd.template on<Mouse::Event>().invoke({ 
        static_cast<uint32_t>(button) | static_cast<uint32_t>(action == GLFW_PRESS ? Mouse::UP : Mouse::DOWN), 
        static_cast<int>(x), 
        static_cast<int>(y) 
    });
}