#pragma once
#include <common/settings.h>
#include <engine/core.h>
#include <engine/window.h>
#include <engine/device.h>
#include <engine/swap.h>
#include <engine/world.h>
#include <engine/render.h>
#include <assets/texture.h>
#include <assets/material.h>
#include <assets/mesh.h>
#include <nodes/node.h>
#include <nodes/camera.h>
#include <nodes/instance.h>
#include <nodes/light.h>

namespace arawn {
	struct Engine {
		Engine(const Settings& info);
		~Engine() noexcept;

		Engine(Engine&& engine) noexcept;
		Engine& operator=(Engine&& engine) noexcept;
		
		Engine(const Engine& engine) noexcept = delete;
		Engine& operator=(const Engine& engine) noexcept = delete;

		void configure(const Settings& value);
		
		bool closed() const;
		void process();

		template<class T> Handle<T> create(const T::CreateInfo& info);
		template<class T> void destroy(Handle<T> hnd);
		
	private:
		Settings settings;
		engine::Core core;
		engine::Window window;
		engine::Device device;
		engine::Swap swap;
		engine::World world;
		engine::Render render;
	};


	// NOTE: differentiating between assets vs nodes might be a mistake
	// it makes it tricky to create dynamic textures/materials/meshes that require updates each frame
	// a better solution would be to simply register dynamic vs static resuources
	// everything is a resource that can either be stored in the dynamic or static pool
	// how that effects the handle?
	// 	could use a single bit to determine which pool static/dynamic to read from for each resource?

	// each resource creates blocks of data in various locations
	
	// textures can remain 


	template<> [[nodiscard]] Handle<assets::Texture> Engine::create(const assets::Texture::CreateInfo&);
	template<> void Engine::destroy(Handle<assets::Texture>);

	template<> [[nodiscard]] Handle<assets::Material> Engine::create(const assets::Material::CreateInfo&);
	template<> void Engine::destroy(Handle<assets::Material>);

	template<> [[nodiscard]] Handle<assets::Mesh> Engine::create(const assets::Mesh::CreateInfo&);
	template<> void Engine::destroy(Handle<assets::Mesh>);



	template<> [[nodiscard]] Handle<nodes::Node> Engine::create(const nodes::Node::CreateInfo&);
	template<> void Engine::destroy(Handle<nodes::Node>);

	template<> [[nodiscard]] Handle<nodes::Light> Engine::create(const nodes::Light::CreateInfo&);
	template<> void Engine::destroy(Handle<nodes::Light>);

	template<> [[nodiscard]] Handle<nodes::Instance> Engine::create(const nodes::Instance::CreateInfo&);
	template<> void Engine::destroy(Handle<nodes::Instance>);

	template<> [[nodiscard]] Handle<nodes::Camera> Engine::create(const nodes::Camera::CreateInfo&);
	template<> void Engine::destroy(Handle<nodes::Camera>);
}


/*
		struct Program { VK_TYPE(VkPipeline) pipeline; VK_TYPE(VkPipelineLayout) layout; };

		// --------- domains --------- 
		struct Scene {
			struct {
				VK_TYPE(VkBuffer) buffer;
				VK_TYPE(VmaAllocation) memory;
				void* data; 
				uint64_t capacity, alloc, retire;
	
				VK_TYPE(VkSemaphore) semaphore;
				VK_TYPE(VkFence) guard;
			} staging;
			struct {
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } vertices;
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } indices;
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } manifest;
			} geometry;
			struct {
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } manifest;
			} material;
			struct {
				struct { VK_TYPE(VkImage) image; VK_TYPE(VmaAllocation) memory; } images;
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } manifest;
			} texture;
			struct {
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } manifest;
			} light;
			struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } cluster;
		};
		
		struct Shadow {
			struct { VK_TYPE(VkImage) image; VK_TYPE(VmaAllocation) memory; VK_TYPE(VkImageView) view; } atlas;
			struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } source;
		};
		struct Reflect {
			struct { VK_TYPE(VkImage) image; VK_TYPE(VmaAllocation) memory; VK_TYPE(VkImageView) view; } cubemaps;
			struct { VK_TYPE(VkImage) image; VK_TYPE(VmaAllocation) memory; VK_TYPE(VkImageView) view; } target; // render here 
		};

		struct Frame {
			VK_TYPE(VkFence) inFlight;
			
			struct {
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } camera;
			} world;

			struct {
				// local passes
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } genHiZ;      // generate the HZB pyramid from frame n-1
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } earlyCull;   // test meshes against HZB pyramid
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } depth;       // render early visible meshes to depth pass
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } genClusters; // generate light clusters from depth
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } lightCull;   // re test occluded meshes in frame n-1 with frame n
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } lateCull;    // cull lights against clusters	
				// external pass syncs
				struct { VK_TYPE(VkSemaphore) finished; } earlyFwd;                                                    // write color attachment with early visible meshes, depth readonly
				struct { VK_TYPE(VkSemaphore) finished; } lateFwd;	                                                   // write color attachmnt with late visible meshes, depth readwrite
				struct { VK_TYPE(VkSemaphore) finished; } acquireI;                                                    // acquire image to present to
				struct { VK_TYPE(VkSemaphore) finished; } postprocess;                                                 // copies frame to swapchain surface 
			} pass;
			
			struct {
				struct { VK_TYPE(VkImage) image; VK_TYPE(VkImageView) view; VK_TYPE(VmaAllocation) memory; } hiZ;
				struct { VK_TYPE(VkImage) image; VK_TYPE(VkImageView) view; VK_TYPE(VmaAllocation) memory; } color;
				struct { VK_TYPE(VkImage) image; VK_TYPE(VkImageView) view; VK_TYPE(VmaAllocation) memory; } depth;
			} attachment;
			
			struct {
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } cluster;
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } light;
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } early;
				struct { VK_TYPE(VkBuffer) buffer; VK_TYPE(VmaAllocation) memory; } late;
			} cull;
		
			

			struct Container {
				Container(Engine& engine);
				~Container();
				Container(Container&& other);
				Container& operator=(Container&& other);
				Container(const Container&) = delete;
				Container& operator=(const Container&) = delete;
				
				struct {
					struct { } genHiZPass;
					struct { } earlyCullPass;
					struct { } depthPass;
					struct { } genClustersPass;
					struct { } lightCullPass;
					struct { } lateCullPass;
				} pass;

				struct {
					struct { } hiZ;
					struct { } color;
					struct { } depth;
				} attachment;

				struct {
					struct { } cluster;
					struct { } light;
					struct { } early;
					struct { } late;
				} cull;

				uint32_t index, count, version;
				Frame* frames;
			};
			
		};

		struct Swap {
			VK_TYPE(VkSemaphore) finished;
			struct { VK_TYPE(VkImage) image; VK_TYPE(VkImageView) view; VK_TYPE(VkSemaphore) ready; } surface;
		
			
			struct Container {
				Container(Engine& engine);
				~Container();
				Container(Container&& other);
				Container& operator=(Container&& other);
				Container(const Container&) = delete;
				Container& operator=(const Container&) = delete;

				VK_TYPE(VkSwapchainKHR) chain;

				uint32_t index, count, version;
				Swap* frames;
			};
		};
		// --------- passes --------- 
		struct Render {
			struct {
				struct { VK_TYPE(VkCommandBuffer) cmd; } earlyFwd; 
				struct { VK_TYPE(VkCommandBuffer) cmd; } lateFwd;
			} pass;

			struct Container {
				Container(Engine& engine);
				~Container();
				Container(Container&& other);
				Container& operator=(Container&& other);
				Container(const Container&) = delete;
				Container& operator=(const Container&) = delete;

				struct {
					struct { } earlyFwd; 
					struct { } lateFwd;
				} pass;

				uint32_t index, count, version;
				Render* frames;
			};
		};
		struct Present {
			struct {
				struct { VK_TYPE(VkCommandBuffer) cmd; } copy;
			} pass;

			struct Container {
				Container(Engine& engine);
				~Container();
				Container(Container&& other);
				Container& operator=(Container&& other);
				Container(const Container&) = delete;
				Container& operator=(const Container&) = delete;

				struct {
					struct { } copy;
				} pass;
				
				uint32_t index, count, version;
				Present* frames;
			};
		};

*/