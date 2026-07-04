#pragma once
#include <ecs.h>
#include <vulkan.h>
#include <memory_resource>

namespace Arawn { // settings.h
	enum class DisplayMode { WINDOWED, FULLSCREEN, EXCLUSIVE };
	enum class BufferingMode { DOUBLE, TRIPLE };
	enum class VsyncMode { DISABLED, ENABLED };
	enum class LowLatencyMode { DISABLED, BALANCED, ENABLED };
	enum class AntiAliasing { DISABLED, MSAA_2, MSAA_4, FXAA_2, FXAA_4, TAA };
	
	struct DisplayInfo {
		const char* gpu;
		struct { uint32_t x, y; } resolution = { 800, 600 };
		uint32_t refreshRate = 60;
		DisplayMode mode = DisplayMode::WINDOWED;
		VsyncMode vsync = VsyncMode::ENABLED;
		LowLatencyMode latency = LowLatencyMode::BALANCED;
		BufferingMode buffering = BufferingMode::DOUBLE;
	};
	
	struct AppInfo {
		const char* title = "Arawn App";
		struct { uint32_t major, minor, patch; } version;
	};
}





namespace Arawn {
	class Engine {
	public:
		Engine(const AppInfo& app = {}, const DisplayInfo& display = {});
		~Engine() noexcept;

		Engine(Engine&& engine) noexcept = delete;
		Engine& operator=(Engine&& engine) noexcept = delete;
		
		Engine(const Engine& engine) noexcept = delete;
		Engine& operator=(const Engine& engine) noexcept = delete;
		
		bool closed() const;
		void render();

	private:
		VK_ENUM(VkFormat) findFormat(const std::vector<VK_ENUM(VkFormat)>& candidates, VK_ENUM(VkImageTiling) tiling, VK_ENUM(VkFormatFeatureFlags) flags) const;

		struct State : AppInfo, DisplayInfo { 
			struct { 
				VK_ENUM(VkFormat) format; 
				VK_ENUM(VkColorSpaceKHR) colorSpace;
			} surface;
		};

		struct Queue { 
			uint32_t family, index;
			VK_TYPE(VkQueue) queue;
			VK_TYPE(VkCommandPool) pool;
		};
		
		template<typename T> struct Domain { uint32_t count, index; T *data; };

		struct Shared {
			struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; void* data; } staging;
			struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } vbo, ebo, ubo;
		};
		struct Frame {
			VK_TYPE(VkFence) inFlight;
			VK_TYPE(VkSemaphore) imageAvailable;
			VK_TYPE(VkSemaphore) forwardFinished;
			struct { VK_TYPE(VkImage) image; VK_TYPE(VmaAllocation) memory; VK_TYPE(VkImageView) view; } colorAttachment, depthAttachment;
		};
		struct Swap {
			VK_TYPE(VkSemaphore) postprocessFinished;
			VK_TYPE(VkImage) image;
		};

		template<typename T>
		struct Pass { uint32_t count; T* data; };
		
		struct Depth { // graphics queue
			VK_TYPE(VkCommandBuffer) cmd;
			void record(const State& state, Shared& shared, Frame& frame); // this set of resources domains is the context
		};
		struct Forward { // graphics queue
			VK_TYPE(VkCommandBuffer) cmd;
			void record(const State& state, Shared& shared, Frame& frame);
		};
		
		struct Present { // present queue
			VK_TYPE(VkCommandBuffer) cmd;
			void record(const State& state,  Shared& shared, Frame& frame, Swap& swap);
		};

		State state;

		VK_TYPE(VkInstance) instance;
#if ARAWN_DEBUG
		VK_TYPE(VkDebugUtilsMessengerEXT) messenger;
#endif
		
		VK_TYPE(GLFWwindow*) window;
		VK_TYPE(VkSurfaceKHR) surface;
		
		VK_TYPE(VkPhysicalDevice) gpu;
		VK_TYPE(VkDevice) device;
		struct { Queue graphics, compute, transfer, present; } queue;
		VK_TYPE(VmaAllocator) allocator;
		
		VK_TYPE(VkSwapchainKHR) swapchain;

		std::pmr::monotonic_buffer_resource cache;

		struct {
			Shared shared;
			Domain<Frame> frame;
			Domain<Swap> swap;
			// TODO: async context
		} context;
		
		struct {
			// TODO: depth pass
			// TODO: mesh culling pass
			// TODO: light culling pass
			Pass<Forward> forward;
			Pass<Present> present;
		} pass;
	};
}