#pragma once
#include <memory>
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Arawn {
	enum class DisplayMode { WINDOWED, FULLSCREEN, EXCLUSIVE };
	enum class BufferingMode { DOUBLE, TRIPLE };
	enum class VsyncMode { NONE, VYSNC };
	struct EngineSettings { 
		const char* appName;
		uint32_t appVersion;
		const char* engineName;
		uint32_t engineVersion;
		const char* gpu;
	};

	struct DisplaySettings {
		struct { uint32_t x, y; } resolution;
		
		DisplayMode display;
		BufferingMode buffering;
		uint32_t refreshRate;

	};

	enum class MeshQuality { LOW, MEDIUM, HIGH };
	enum class TextureQuality  { LOW, MEDIUM, HIGH };
	enum class ShadowQuality { NONE, LOW, MEDIUM, HIGH };
	enum class AmbientOcclusionQuality { NONE, LOW, MEDIUM, HIGH };
	enum class GlobalIlluminationQuality { NONE, LOW, MEDIUM, HIGH };
	enum class AntiAliasing { NONE, FXAA, MSAA_2X, MSAA_4X, TAA };

	struct GraphicsSettings {
		MeshQuality mesh;
		TextureQuality texture;
		ShadowQuality shadow;
		AmbientOcclusionQuality ao;
		GlobalIlluminationQuality gi;
	};

	class Engine {
	public:
		struct Queue { 
			uint32_t family, index;
			vk::Queue queue;
		};
		using Window = std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)>;
	
	public:
		Engine(const EngineSettings& engine = {}, const DisplaySettings& display = {}, const GraphicsSettings& graphics = {});

		void recreate();

		void apply(const GraphicsSettings& graphics);

		void apply(const DisplaySettings& display) { apply(display, settings.graphics); }
		void apply(const DisplaySettings& display, const GraphicsSettings& graphics);
		
	private:
		vk::raii::Instance createInstance();
		vk::raii::PhysicalDevice selectGPU();
		vk::raii::Device createDevice();
		Window createWindow();
		vk::raii::SurfaceKHR createSurface();
		vk::raii::SwapchainKHR createSwapchain();

		struct {
			EngineSettings engine;
			DisplaySettings display;
			GraphicsSettings graphics;
		} settings;

		vk::raii::Context context;
		vk::raii::Instance instance;
		vk::raii::PhysicalDevice gpu;
		vk::raii::Device device;
		Queue graphics, compute, transfer, async;

		Window window;
		vk::raii::SurfaceKHR surface;
		vk::raii::SwapchainKHR swapchain;



	};
}


