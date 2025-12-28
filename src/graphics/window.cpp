#define ARAWN_IMPLEMENTATION
#include <graphics/engine.h>
#include <graphics/window.h>
#include <core/settings.h>
#include <algorithm>
#include <cassert>

const GLFWvidmode* selectVideoMode(GLFWmonitor* monitor, int width, int height)
{
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

GLFW_WINDOW Arawn::Window::createWindow(const char* name)
{
    using x = Settings::data_type;
    
    auto& res = Arawn::settings.get<Arawn::Resolution>();
    
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    return glfwCreateWindow(res.width, res.height, name, nullptr, nullptr);
}

VkSurfaceKHR Arawn::Window::createSurface(GLFW_WINDOW window)
{
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(engine.instance, window, nullptr, &surface);
    return surface;
}

Arawn::Window::Window(const char* name) : window(createWindow(name)), swapchain(createSurface(window))
{
    assert(window);

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, key_callback);
    //glfwSetCharCallback(window, char_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwSetCursorPosCallback(window, mouse_move_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    setDisplayMode(settings.get<DisplayMode>());

    uptime = std::chrono::high_resolution_clock::now();

    auto [width, height] = size();
    swapchain.recreate(width, height);
}

Arawn::Window::~Window()
{
    if (window != nullptr)
    {
        glfwDestroyWindow(window);
    }
}


Arawn::Window::Window(Window&& other) : window(other.window), swapchain(std::move(other.swapchain))
{
    glfwSetWindowUserPointer(window, this);
    
    other.window = nullptr;
}

Arawn::Window& Arawn::Window::operator=(Window&& other)
{
    if (this == &other)
    {
        return *this;
    }
    
    swapchain = std::move(other.swapchain);
    
    if (window != nullptr)
    {
        glfwDestroyWindow(window);
    }

    window = other.window;
    other.window = nullptr;

    glfwSetWindowUserPointer(window, this);

    return *this;
}


/*
void Window::set_title(const char* title)
{
    glfwSetWindowTitle(window, title);
}
*/

void Arawn::Window::resize(uint32_t x, uint32_t y)
{
    glfwSetWindowSize(window, x, y);
}

auto Arawn::Window::size() const -> std::pair<uint32_t, uint32_t> {
    int x, y;
    glfwGetWindowSize(window, &x, &y);
    return { x, y };
}

auto Arawn::Window::enumResolutions(float ratio, float precision) const -> std::vector<std::pair<uint32_t, uint32_t>> {
    GLFWmonitor* monitor = glfwGetWindowMonitor(window);
    if (monitor == nullptr) monitor = glfwGetPrimaryMonitor();

    int count;
    const GLFWvidmode* video_modes = glfwGetVideoModes(monitor, &count);
    std::vector<std::pair<uint32_t, uint32_t>> resolutions(count);
    
    for (int i = 0; i < count; ++i)
        resolutions[i] = { video_modes[i].width, video_modes[i].height }; 
    
    auto end = resolutions.end();
    end = std::remove_if(resolutions.begin(), end, [=](auto& res)
    { 
        auto [x, y] = res;
        return std::abs(((float)x / y) - ratio) > precision; 
    });

    std::sort(resolutions.begin(), end, [](auto& lhs, auto& rhs)
    { 
        auto [lhs_x, lhs_y] = lhs;
        auto [rhs_x, rhs_y] = rhs;
        if (lhs_x != rhs_x) return lhs_x < rhs_x; else return lhs_y < rhs_y;
    });

    end = std::unique(resolutions.begin(), end);
    end = std::remove_if(resolutions.begin(), end, [](auto& res)
    {
        auto [x, y] = res;
        return x < 800 || y < 600;
    }); // min size
    
    resolutions.erase(end, resolutions.end());
    return resolutions;
}

void Arawn::Window::setDisplayMode(DisplayMode::Enum displayMode)
{
    switch (displayMode)
    {
        case DisplayMode::EXCLUSIVE: {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            const GLFWvidmode* vid_mode = selectVideoMode(monitor, width, height); 
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
Arawn::DisplayMode::Enum Arawn::Window::getDisplayMode() const {
    // checks features of 
    if (glfwGetWindowMonitor(window) != nullptr) 
        return DisplayMode::EXCLUSIVE;
    if (glfwGetWindowAttrib(window, GLFW_FLOATING) == GLFW_TRUE)
        return DisplayMode::FULLSCREEN;
    //if (glfwGetWindowAttrib(window, GLFW_DECORATED) == GLFW_TRUE) 
    return DisplayMode::WINDOWED;
}

void Arawn::Window::poll()
{
    glfwPollEvents();
    time current_frame = std::chrono::high_resolution_clock::now();
    //Dispatcher<Update>::invoke(Update{ std::chrono::duration<float, std::chrono::seconds::period>(current_frame - uptime).count() });
    uptime = current_frame;
}

void Arawn::Window::close() {
    
}

auto Arawn::Window::closed() const -> bool {
    return glfwWindowShouldClose(window);
}

auto Arawn::Window::minimized() const -> bool {
    int x, y;
    glfwGetWindowSize(window, &x, &y);
    return x == 0 || y == 0;
}
/*
auto Window::mouse_position() const -> glm::vec2 {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return { x, y };
}
*/

void Arawn::Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{ 
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    //wnd.template on<Key::Event>().invoke({ key, action });
}

void Arawn::Window::mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{ 
    double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    glfwGetCursorPos(window, &x, &y);
    //wnd.template on<Mouse::Event>().invoke({ Mouse::SCROLL, yoffset > 0 ? Mouse::UP : Mouse::DOWN, static_cast<int>(x), static_cast<int>(y) });
}

void Arawn::Window::mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{ 
    double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    glfwGetCursorPos(window, &x, &y);
    //wnd.template on<Mouse::Event>().invoke({ Mouse::NONE, 0, static_cast<int>(x), static_cast<int>(y) });
}

void Arawn::Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{ 
    double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    glfwGetCursorPos(window, &x, &y);
    //wnd.template on<Mouse::Event>().invoke({ button, action, static_cast<int>(x), static_cast<int>(y) });
}