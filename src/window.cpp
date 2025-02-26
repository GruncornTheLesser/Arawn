#define VK_IMPLEMENTATION
#include "window.h"
#include <algorithm>

void key_callback(VK_TYPE(GLFWwindow*) glfwWindow, int key, int scancode, int action, int mods);
void char_callback(VK_TYPE(GLFWwindow*) glfwWindow, unsigned int codepoint);
void mouse_move_callback(VK_TYPE(GLFWwindow*) glfwWindow, double xpos, double ypos);
void mouse_scroll_callback(VK_TYPE(GLFWwindow*) glfwWindow, double xoffset, double yoffset);
void mouse_button_callback(VK_TYPE(GLFWwindow*) glfwWindow, int button, int action, int mods);

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

Window::Window()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vid_mode = select_vid_mode(monitor, settings.resolution.x, settings.resolution.y); // resolution must match monitor video mode
    
    switch (settings.display_mode) {
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

/*
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
*/

void Window::set_title(const char* title) {
    glfwSetWindowTitle(window, title);
}

void Window::set_resolution(glm::uvec2 res) {
    glfwSetWindowSize(window, res.x, res.y);
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
    
    auto end = resolutions.end();
    end = std::remove_if(resolutions.begin(), end, [=](auto& res) { return std::abs(((float)res.x / res.y) - ratio) > precision; });
    std::sort(resolutions.begin(), end, [](auto& lhs, auto& rhs) { if (lhs.x != rhs.x) return lhs.x < rhs.x; else return lhs.y < rhs.y; });
    end = std::unique(resolutions.begin(), end);
    end = std::remove_if(resolutions.begin(), end, [](auto& res) { return res.x < 800 || res.y < 600; }); // min size
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



void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) { 
	auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    wnd.template on<Key::Event>().invoke({ key, action });
}

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { 
	double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
	glfwGetCursorPos(window, &x, &y);
    wnd.template on<Mouse::Event>().invoke({ Mouse::SCROLL, yoffset > 0 ? Mouse::UP : Mouse::DOWN, static_cast<int>(x), static_cast<int>(y) });
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos) { 
	double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
	glfwGetCursorPos(window, &x, &y);
    wnd.template on<Mouse::Event>().invoke({ 0, 0, static_cast<int>(x), static_cast<int>(y) });
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) { 
	double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
	glfwGetCursorPos(window, &x, &y);
    wnd.template on<Mouse::Event>().invoke({ button, action, static_cast<int>(x), static_cast<int>(y) });
}