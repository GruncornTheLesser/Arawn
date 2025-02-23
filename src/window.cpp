#define VK_IMPLEMENTATION
#include "window.h"

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
Key::State get_key_state(int action) {

}

const GLFWvidmode* select_vid_mode(GLFWmonitor* monitor, int width, int height) {
    int count;
    const GLFWvidmode* cur = glfwGetVideoModes(monitor, &count);
    const GLFWvidmode* end = cur + count;
    const GLFWvidmode* vid_mode = nullptr;
    do {
        if (cur->width < width) continue;
        if (cur->height < height) continue;
        vid_mode = cur;
        break;
    } while(++cur < end);
    do {
        if (cur->width < width) continue;
        if (cur->height < height) continue;
        if (cur->width > vid_mode->width) continue;
        if (cur->height > vid_mode->height) continue;
        vid_mode = cur;
    } while(++cur < end);
    return vid_mode;
}

Window::Window(uint32_t width, uint32_t height, const char* title, DisplayMode display_mode)
{    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    if (display_mode == DisplayMode::WINDOWED) {
        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    } else {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* vid_mode = select_vid_mode(monitor, width, height); 
        if (vid_mode == nullptr) {
            window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        } else {
            window = glfwCreateWindow(vid_mode->width, vid_mode->height, title, monitor, nullptr);
        }

        glfwFocusWindow(window);
    }
    
    GLFW_ASSERT(window);

    glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, key_callback);
	//glfwSetCharCallback(window, char_callback);
	glfwSetScrollCallback(window, mouse_scroll_callback);
	glfwSetCursorPosCallback(window, mouse_move_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetWindowSizeCallback(window, window_resize_callback);
}

Window::~Window() {
    if (window != nullptr) 
        glfwDestroyWindow(window);
}

Window::Window(Window&& other) {
    window = other.window;
    other.window = nullptr;
}

Window& Window::operator=(Window&& other) {
    if (this != &other) { // if not assigned to self
        if (window != nullptr)
            glfwDestroyWindow(window);
        
        std::swap(window, other.window);
    }
    return *this;
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


auto Window::get_resolution() const -> std::pair<uint32_t, uint32_t> {
    int x, y;
    glfwGetWindowSize(window, &x, &y);
    return { x, y };
}

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) { 
	auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    wnd.template on<Key::Event>().invoke({
        static_cast<uint32_t>(get_key_code(key)) | static_cast<uint32_t>(action == GLFW_PRESS ? Key::UP : Key::DOWN) 
    });
}

//void Window::char_callback(GLFWwindow* window, unsigned int codepoint) { 
//	auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
//
//}

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
void Window::window_resize_callback(GLFWwindow* window, int width, int height) {
	auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
	//wnd.template on<>().invoke()
}