#define VK_IMPLEMENTATION
#include "window.h"

Window::Window(uint32_t width, uint32_t height, DisplayMode display)
{    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    GLFW_ASSERT(window = glfwCreateWindow(width, height, "", nullptr, nullptr));
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