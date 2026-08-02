#define ARAWN_INCLUDE_VULKAN
#include <engine/window.h>
#include <algorithm>
#include <cstring>

using namespace arawn;

/* selects a video mode from the monitor that most closely matches the target resolution and refresh rate*/
auto selectVideoMode(GLFWmonitor* monitor, int targetWidth, int targetHeight, int targetRefresh)->const GLFWvidmode& {
	int count;
	const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);
	return *std::ranges::min_element(std::span{ modes, modes + count }, {}, [&](const GLFWvidmode& mode)->uint32_t { 
		uint32_t delta_x = std::abs(mode.width - targetWidth); 
		uint32_t delta_y = std::abs(mode.height - targetHeight); 
		uint32_t refreshRateDiff = std::abs(mode.refreshRate - targetRefresh); 

		return (delta_x + delta_y) * 10000 + refreshRateDiff;
	});
};

/* used on init to get the primary monitor or the monitor passed in by the selection arg */
auto selectDefaultMonitor(const char* selection) -> GLFWmonitor* {
	int count;
	GLFWmonitor** monitorData = glfwGetMonitors(&count);
	std::span monitors{ monitorData, monitorData + count };

	if (selection != nullptr) {
		auto monitorMatch = [&](GLFWmonitor* monitor) -> bool { return strcmp(glfwGetMonitorName(monitor), selection); };
		if (auto it = std::ranges::find_if(monitors, monitorMatch); it != monitors.end()) return *it;
	}

	return glfwGetPrimaryMonitor();
};

/* used on recreate to get the primary monitor closest to the windows current position or the monitor passed in by the selection arg */
auto selectCurrentMonitor(GLFWwindow* window, const char* selection)->GLFWmonitor* {
	int count;
	GLFWmonitor** monitorData = glfwGetMonitors(&count);
	std::span monitors{ monitorData, monitorData + count };

	if (selection != nullptr) {
		auto monitorMatch = [&](GLFWmonitor* monitor) -> bool { return strcmp(glfwGetMonitorName(monitor), selection); };
		if (auto it = std::ranges::find_if(monitors, monitorMatch); it != monitors.end()) return *it;
	}

	struct { int x, y, width, height; } win;
	glfwGetWindowPos(window, &win.x, &win.y);
	glfwGetWindowSize(window, &win.width, &win.height);

	return *std::ranges::max_element(monitors, {}, [&](GLFWmonitor* monitor)->uint32_t {
		struct { int x, y, width, height; } mon;
		glfwGetMonitorPos(monitor, &mon.x, &mon.y);
		
		const GLFWvidmode& mode = *glfwGetVideoMode(monitor);
		mon.width = mode.width;
		mon.height = mode.height;

		int x1 = std::max(win.x, mon.x);
		int y1 = std::max(win.y, mon.y);
		int x2 = std::min(win.x + win.width, mon.x + mode.width);
		int y2 = std::min(win.y + win.height, mon.y + mode.height);

		return std::max(0, x2 - x1) * std::max(0, y2 - y1);
	});
};

/* resets the monito video mode back to the maximum */
auto resetVideoMode(GLFWwindow* window, GLFWmonitor* monitor) {
	int count;
	const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);
	const GLFWvidmode& mode = modes[count - 1];

	glfwSetWindowMonitor(
		window, 
		monitor, 
		0, 0, 
		mode.width, mode.height,
		mode.refreshRate
	);
};

void engine::Window::create(const Core& core, const Settings& info) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_REFRESH_RATE, info.refreshRate);
	
	window = nullptr;

	switch(info.display) {
	case DisplayMode::WINDOWED: {
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
		
		window = glfwCreateWindow(
			info.resolution.x, info.resolution.y,
			info.title,
			nullptr,
			nullptr
		);
		break;
	}
	case DisplayMode::FULLSCREEN: {
		GLFWmonitor* monitor = selectDefaultMonitor(info.monitor);
		const GLFWvidmode& mode = *glfwGetVideoMode(monitor);
		
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

		window = glfwCreateWindow(
			mode.width, mode.height,
			info.title,
			nullptr,
			nullptr
		);
		break;
	}
	case DisplayMode::EXCLUSIVE: {
		GLFWmonitor* monitor = selectDefaultMonitor(info.monitor);
		const GLFWvidmode& mode = selectVideoMode(monitor, info.resolution.x, info.resolution.y, info.refreshRate);
		
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
		glfwWindowHint(GLFW_REFRESH_RATE, mode.refreshRate);

		window = glfwCreateWindow(
			mode.width, mode.height,
			info.title,
			monitor,
			nullptr
		);
		break;
	}	
	}
	
	if (window == nullptr) [[unlikely]] {
		const char* message;
		glfwGetError(&message);
		ARAWN_THROW(std::format("failed to initialize window: {}", message));
	}

	VK_ASSERT(glfwCreateWindowSurface(core.instance, window, nullptr, &surface));
}

void engine::Window::recreate(const Core& core, const Settings& info) {
	switch(info.display) {
	case DisplayMode::WINDOWED: {
		GLFWmonitor* monitor = glfwGetWindowMonitor(window); 
		if (monitor != nullptr) {
			resetVideoMode(window, monitor);
		}
		
		glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
		glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_FALSE);
		
		glfwSetWindowMonitor(
			window, 
			nullptr, 
			100, 100, 
			info.resolution.x, info.resolution.y,
			info.refreshRate
		);
		break;
	}
	case DisplayMode::FULLSCREEN: {
		GLFWmonitor* monitor = glfwGetWindowMonitor(window); 
		if (monitor != nullptr) resetVideoMode(window, monitor);
		else monitor = selectCurrentMonitor(window, info.monitor);
		
		glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
		glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
		
		struct { int x, y; } mon;
		glfwGetMonitorPos(monitor, &mon.x, &mon.y);
		const GLFWvidmode& mode = *glfwGetVideoMode(monitor);
		
		glfwSetWindowMonitor(
			window,
			nullptr,
			mon.x, mon.y,
			mode.width, mode.height,
			info.refreshRate
		);

		break;
	}
	case DisplayMode::EXCLUSIVE: {
		GLFWmonitor* monitor = glfwGetWindowMonitor(window); 
		if (monitor == nullptr) monitor = selectCurrentMonitor(window, info.monitor);	

		const GLFWvidmode& mode = selectVideoMode(monitor, info.resolution.x, info.resolution.y, info.refreshRate);

		glfwSetWindowMonitor(
			window,
			monitor,
			0, 0,
			mode.width, mode.height,
			mode.refreshRate
		);

		break;
	}
	}
}

void engine::Window::destroy(const Core& core) {
	vkDestroySurfaceKHR(core.instance, surface, nullptr);
	glfwDestroyWindow(window);
}
