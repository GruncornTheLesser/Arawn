#pragma once
#define ARAWN_IMPLEMENTAION
#include <filesystem>
#include <mdspan>
#include <vulkan.h>
#include <memory_resource>
#include <span>

namespace Arawn { // settings.h
	enum class DisplayMode { WINDOWED, FULLSCREEN, EXCLUSIVE };
	enum class BufferingMode { DOUBLE, TRIPLE };
	enum class VsyncMode { DISABLED, ENABLED };
	enum class LowLatencyMode { DISABLED, BALANCED, ENABLED };
	enum class AntiAliasing { DISABLED, MSAA_2, MSAA_4, FXAA_2, FXAA_4, TAA };
	
	struct DisplayInfo {
		const char* gpu = nullptr;
		const char* monitor = nullptr;
		struct { uint32_t x, y; } resolution = { 800, 600 };
		uint32_t refreshRate = 60;
		DisplayMode display = DisplayMode::WINDOWED;
		VsyncMode vsync = VsyncMode::ENABLED;
		LowLatencyMode latency = LowLatencyMode::BALANCED;
		BufferingMode buffering = BufferingMode::DOUBLE;
	};
	
	struct AppInfo {
		const char* title = "Arawn App";
		struct { uint32_t major, minor, patch; } version;
	};
}

// NOTE: must be inline for allocations to work
// raw heap allocations before main() are odd. 
// inline allows the compiler to defer the initialization to the start of main()
// this is called the static order initialization fiasco

namespace Arawn {
	
	inline extern class Engine {
		struct State : AppInfo, DisplayInfo { 
			struct { 
				VK_ENUM(VkFormat) format; 
				VK_ENUM(VkColorSpaceKHR) colorSpace;
			} surface;
		};
		struct QueueIndex { uint32_t family, index; };
		struct QueueIndices { QueueIndex graphics, compute, transfer, present; };
		struct Queue : QueueIndex { VK_TYPE(VkQueue) queue; VK_TYPE(VkCommandPool) pool; };

		template<typename Dom_T>
		struct Domain : std::mdspan<Dom_T, std::extents<uint32_t, Dom_T::buffering>> { 
			Domain() { }
			Domain(Dom_T* frames, uint32_t count) : std::mdspan<Dom_T, std::extents<uint32_t, Dom_T::buffering>>(frames, count), index(0) { }
			uint32_t index;
		};
	
		template<typename Pass_T, typename ... Dom_Ts>
		struct Pass : std::mdspan<Pass_T, std::extents<uint32_t, Dom_Ts::buffering...>> { 
			Pass() { }
			Pass(Pass_T* frames, const std::array<uint32_t, sizeof...(Dom_Ts)>& counts) : std::mdspan<Pass_T, std::extents<uint32_t, Dom_Ts::buffering...>>(frames, counts) { }
		};
	
		// ------------- domain frames ------------- 
		struct Shared {
			static constexpr std::size_t buffering = 1;
			struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; void* data; } staging;
			struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } vbo, ebo, ubo;
		};
		struct Frame {
			static constexpr std::size_t buffering = std::dynamic_extent;
			VK_TYPE(VkFence) inFlight;
			VK_TYPE(VkSemaphore) imageAvailable;
			VK_TYPE(VkSemaphore) forwardFinished;
			struct { VK_TYPE(VkImage) image; VK_TYPE(VmaAllocation) memory; VK_TYPE(VkImageView) view; } colorAttachment, depthAttachment;
		};
		struct Swap {
			static constexpr std::size_t buffering = std::dynamic_extent;
			VK_TYPE(VkSemaphore) postprocessFinished;
			VK_TYPE(VkImage) image;
		};
	
		// ------------- pass frames ------------- 
		struct Forward { // graphics queue
			VK_TYPE(VkCommandBuffer) cmd;
			void record(const State& state, const Shared& shared, const Frame& frame);
		};
		struct Present { // present queue
			VK_TYPE(VkCommandBuffer) cmd;
			void record(const State& state, const Shared& shared, const Frame& frame, const Swap& swap);
		};
		
	private:
		Engine(const AppInfo& app, const DisplayInfo& display, QueueIndices&& queueIndices);
	public:
		Engine(const AppInfo& app = {}, const DisplayInfo& display = {}) : Engine(app, display, {}) { }
		~Engine() noexcept;

		Engine(Engine&& engine) noexcept = delete;
		Engine& operator=(Engine&& engine) noexcept = delete;
		
		Engine(const Engine& engine) noexcept = delete;
		Engine& operator=(const Engine& engine) noexcept = delete;
		
		bool closed() const;
		void render();

	private:
		VK_TYPE(VkInstance) createInstance() const;
#ifdef ARAWN_DEBUG
		VK_TYPE(VkDebugUtilsMessengerEXT) createMessenger() const;
#endif
		VK_TYPE(GLFWwindow*) createWindow() const;
		VK_TYPE(VkSurfaceKHR) createSurface() const;
		VK_TYPE(VkPhysicalDevice) selectGPU() const;
		VK_TYPE(VkDevice) createDevice(QueueIndices& queueIndices) const;
		Queue getQueue(const QueueIndex&) const;
		VK_TYPE(VmaAllocator) createAllocator() const;
		VK_TYPE(VkSwapchainKHR) createSwapchain() const;

		VK_ENUM(VkFormat) findFormat(const std::vector<VK_ENUM(VkFormat)>& candidates, VK_ENUM(VkImageTiling) tiling, VK_ENUM(VkFormatFeatureFlags) flags) const;
		
	private:
		State state;

		VK_TYPE(VkInstance) instance;
#ifdef ARAWN_DEBUG
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
		} domain;
	
		struct {
			Pass<Forward, Frame> forward;
			Pass<Present, Frame, Swap> present;
		} pass;
	} engine;
}

/*
	class Texture {
	public:
		static Texture load(std::filesystem::path path);	
		Texture(uint32_t width, uint32_t height, std::byte* data = nullptr);
		
		~Texture();
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        Texture& operator=(Texture&&) noexcept;


	private:
		VK_TYPE(VkImage) image;
		VK_TYPE(VkImageView) view;
		VK_TYPE(VmaVirtualAllocation) memory;
	};
	
	class Buffer {
	public:
		static Buffer load(std::filesystem::path path);
		Buffer(std::byte* data, uint32_t size);
		
		~Buffer();
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept;
        Buffer& operator=(Buffer&&) noexcept;
	private:
		VK_TYPE(VkBuffer) buffer;
		VK_TYPE(VmaVirtualAllocation) memory;
	};

	class Material {
	public:
		static Material load(std::filesystem::path path);
		Material();
	};

	class Mesh {
		struct Meshlet {
			uint32_t index;
			uint32_t count;
			uint32_t material; // handle
		};
	public:
		Mesh(std::filesystem::path path);
		
		~Mesh();
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) noexcept = default;
        Mesh& operator=(Mesh&&) noexcept = default;

	private:
		Buffer vertices;  // vertex buffer object
		Buffer indices;   // element buffer object
		Buffer manifest;  // meshlet buffer object
	};

	class Program {
	public:
		class Shader {
			friend class Program;
		public:
			Shader(std::filesystem::path path);
			~Shader();
            
            Shader(const Shader&) = delete;
            Shader& operator=(const Shader&) = delete;
            Shader(Shader&&) noexcept;
            Shader& operator=(Shader&&) noexcept;

		private:
			VK_TYPE(VkShaderModule) shader;
		};

		Program(Shader comp);
		Program(Shader vert, Shader frag);
		Program(Shader comp, Shader geom, Shader frag);

		~Program();
        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;
        Program(Program&&) noexcept;
        Program& operator=(Program&&) noexcept;

	private:
		VK_TYPE(VkPipeline) pipeline;
		VK_TYPE(VkPipelineLayout) layout;
	};
*/