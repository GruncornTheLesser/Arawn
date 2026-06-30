#pragma once
#include <memory>
#include <ecs.h>
#include <vulkan.h>
#include <vector>

namespace Arawn {
	enum class DisplayMode { WINDOWED, FULLSCREEN, EXCLUSIVE };
	enum class BufferingMode { DOUBLE, TRIPLE };
	enum class VsyncMode { DISABLED, ENABLED };
	enum class LowLatencyMode { DISABLED, BALANCED, ENABLED };

	struct Info { 
		const char* appName = "Arawn App";
		uint32_t appVersion;
		const char* gpu;
		struct { uint32_t x, y; } resolution = { 800, 600 };
		uint32_t refreshRate = 60;
		DisplayMode mode = DisplayMode::WINDOWED;
		VsyncMode vsync = VsyncMode::ENABLED;
		LowLatencyMode latency = LowLatencyMode::BALANCED;
		BufferingMode buffering = BufferingMode::DOUBLE;
	};

	class Engine {
		struct State {
			VkPhysicalDevice gpu;
			const GLFWmonitor* monitor;
			struct { uint32_t x, y; } resolution;
			uint32_t refreshrate;
			DisplayMode display;
			VkPresentModeKHR presentMode;
			uint32_t frameCount;
		};
	public:
		Engine(const Info& info = {});
		~Engine();

		Engine(Engine&& engine);
		Engine& operator=(Engine&& engine);
		
		Engine(const Engine& engine) = delete;
		Engine& operator=(const Engine& engine) = delete;
	
	private:
		struct Queue { 
			uint32_t family, index;
			VkQueue queue;
		};
		struct VmaAllocatorDeleter { void operator()(VmaAllocator_T*); };
		using VmaAllocatorRAII = std::unique_ptr<VmaAllocator_T, VmaAllocatorDeleter>;
		
		struct GLFWwindowDeleter { void operator()(GLFWwindow*); };
		using WindowRAII = std::unique_ptr<GLFWwindow, GLFWwindowDeleter>;

		VkInstance instance;
		#if ARAWN_DEBUG
		VkDebugUtilsMessengerEXT messenger;
		#endif
		
		GLFWwindow* window;
		VkSurfaceKHR surface;
		
		VkPhysicalDevice gpu;
		Queue graphics, compute, transfer, present;
		VkDevice device;
		VmaAllocator allocator;
		
		VkSurfaceFormatKHR surfaceFormat;
		VkSwapchainKHR swapchain;
		
		struct {
			VkBuffer buffer;
			VmaAllocation memory;
		} lightIndices;
		
		struct Swap { // swapchain frame
			struct {
				VkImage image;
				VkImageView view;
			} image;
			
		};
		struct Frame {
			struct {
				VkImage image;
				VkImageView view;
				VmaAllocation memory;
			} depth;

			struct {
				VkImage image;
				VkImageView view;
				VmaAllocation memory;
			} color;

			struct {
				VkBuffer buffer;
				VmaAllocation memory;
			} lightData;

			struct {
				VkBuffer buffer;
				VmaAllocation memory;
			} lightIndices;
		};

		std::vector<VkCommandBuffer> cmds;
		State state;
	};
}