#pragma once
#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <ecs.h>

namespace Arawn {
	enum class DisplayMode { WINDOWED, FULLSCREEN, EXCLUSIVE };
	enum class BufferingMode { DOUBLE, TRIPLE };
	enum class VsyncMode { DISABLED, ENABLED };
	enum class LowLatencyMode { DISABLED, BALANCED, ENABLED };
	struct AppSettings { 
		const char* appName = "";
		uint32_t appVersion;
		// API version??
		// extensions??
	};
	
	struct DisplaySettings {
		const char* gpu = nullptr;
		DisplayMode mode = DisplayMode::WINDOWED;
		struct { uint32_t x, y; } resolution = { 800, 600 };
		uint32_t refreshRate = 60;
		VsyncMode vsync = VsyncMode::ENABLED;
		LowLatencyMode latency = LowLatencyMode::BALANCED;
		BufferingMode buffering = BufferingMode::DOUBLE;
	};

	enum class MeshQuality { LOW, MEDIUM, HIGH };
	enum class TextureQuality  { LOW, MEDIUM, HIGH };
	enum class ShadowQuality { NONE, LOW, MEDIUM, HIGH };
	enum class AmbientOcclusionQuality { NONE, LOW, MEDIUM, HIGH };
	enum class GlobalIlluminationQuality { NONE, LOW, MEDIUM, HIGH };
	enum class ScreenSpaceReflectionQuality { NONE, LOW, MEDIUM, HIGH };
	enum class AntiAliasing { NONE, FXAA, MSAA_2X, MSAA_4X, TAA };

	struct GraphicsSettings {
		MeshQuality mesh;
		TextureQuality texture;
		ShadowQuality shadow;
		AmbientOcclusionQuality ambientOcclusion;
		GlobalIlluminationQuality globalIllumination;
		ScreenSpaceReflectionQuality ssr;
	};

	struct Settings {
		AppSettings app = {};
		DisplaySettings display = {};
		GraphicsSettings graphics = {};
	};

	class Engine {
	public:
		Engine(const Settings& settings = {});
		Engine& operator=(const Settings& settings);
		Engine& operator=(const DisplaySettings& settings);
		Engine& operator=(const GraphicsSettings& settings);

	private:
		struct Queue { 
			uint32_t family, index;
			vk::Queue queue;
		};
		struct VmaAllocatorDeleter { void operator()(VmaAllocator_T*); };
		using VmaAllocatorRAII = std::unique_ptr<VmaAllocator_T, VmaAllocatorDeleter>;
		
		struct GLFWwindowDeleter { void operator()(GLFWwindow*); };
		using WindowRAII = std::unique_ptr<GLFWwindow, GLFWwindowDeleter>;

		
		vk::raii::Instance createInstance();
		#if ARAWN_DEBUG
		vk::raii::DebugUtilsMessengerEXT createDebugMessenger();
		#endif
		WindowRAII createWindow();
		vk::raii::SurfaceKHR createSurface();
		vk::raii::PhysicalDevice selectGPU();
		vk::raii::Device createDevice();
		VmaAllocatorRAII createAllocator();
		vk::raii::SwapchainKHR createSwapchain();
		
		Settings settings;
		
		vk::raii::Context context;
		vk::raii::Instance instance;
		#if ARAWN_DEBUG
		vk::raii::DebugUtilsMessengerEXT debugger;
		#endif
		
		WindowRAII window;
		vk::raii::SurfaceKHR surface;
		
		vk::raii::PhysicalDevice gpu;
		Queue graphics, compute, transfer, present;
		vk::raii::Device device;
		VmaAllocatorRAII allocator;
		
		vk::SurfaceFormatKHR surfaceFormat;
		vk::raii::SwapchainKHR swapchain;

		// ecs::registry<Mesh, Image, Buffer, Program> registry;
	};
}