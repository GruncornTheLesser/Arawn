#pragma once
#include <engine/device.h>
#include <engine/world.h>
#include <nodes/camera.h>
#include <vector>

namespace arawn::engine {
	struct Render {
		void create(const Core& core, const Window& window, const Device& device, const World& world, const Settings& info);
		void recreate(const Core& core, const Window& window, const Device& device, const World& world, const Settings& info);
		void destroy(const Core& core, const Device& device);
		
		struct Frame {
			VK_TYPE(VkFence) inFlight;

			struct {
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } depthTree; // compute/graphic??
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } earlyCull; // compute: frustum cull, occl cull
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } depthOnly; // graphic: depth prepass
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } lateCull;  // compute: light tile cull, mesh cluster culll
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } forward;   // graphic: forward pass
			} pass;

			struct {
				VK_TYPE(VkBuffer) buffer;
				VK_TYPE(VmaAllocation) memory;
				primitives::Camera* mapped;
			} camera;
			
			struct { // mesh instances
				struct { // meshes to draw this frame 
					VK_TYPE(VkBuffer) buffer;
					VK_TYPE(VmaAllocation) memory;
				} manifest;
				struct { // visible from frustum and reproj cull
					VK_TYPE(VkBuffer) buffer;
					VK_TYPE(VmaAllocation) memory;
				} early;
				struct { // visible from cluster cull
					VK_TYPE(VkBuffer) buffer;
					VK_TYPE(VmaAllocation) memory;
				} late;
				struct { // mesh indices per cluster
					VK_TYPE(VkBuffer) buffer;
					VK_TYPE(VmaAllocation) memory;
				} cluster;
			} instance;

			struct { 
				struct {
					VK_TYPE(VkBuffer) buffer;
					VK_TYPE(VmaAllocation) memory;
				} manifest;
				struct {
					VK_TYPE(VkBuffer) buffer;
					VK_TYPE(VmaAllocation) memory;
				} tile;
			} light;

			struct {
				struct { 
					VK_TYPE(VkImage) image;
					VK_TYPE(VmaAllocation) memory;
					VK_TYPE(VkImageView) view;
					VK_TYPE(VkImageView) mips[15]; // depth tree
				} depth;
				struct { 
					VK_TYPE(VkImage) image;
					VK_TYPE(VmaAllocation) memory;
					VK_TYPE(VkImageView) view;
				} color;
			} attachment;

			struct {
				VK_TYPE(VkDescriptorSet) set;
			} descriptor;
		};
		
		struct {
			struct {
				struct {
					VK_TYPE(VkPipeline) pipeline;
					VK_TYPE(VkPipelineLayout) layout;
				} generate;
			} depthTree; // compute/graphic??
			struct { 
				struct {
					VK_TYPE(VkPipeline) pipeline;
					VK_TYPE(VkPipelineLayout) layout;
				} frustumCull;
				struct {
					VK_TYPE(VkPipeline) pipeline;
					VK_TYPE(VkPipelineLayout) layout;
				} occlusionCull; // reprojection cull
			} earlyCull; // compute: frustum cull, occl cull
			struct { 
				struct {
					VK_TYPE(VkPipeline) pipeline;
					VK_TYPE(VkPipelineLayout) layout;
				} depthPass; // rdepth prepass

			} depthOnly; // graphic
			
			struct { 
				struct {
					VK_TYPE(VkPipeline) pipeline;
					VK_TYPE(VkPipelineLayout) layout;
				} generateTiles;
				struct {
					VK_TYPE(VkPipeline) pipeline;
					VK_TYPE(VkPipelineLayout) layout;
				} generateClusters;
				struct {
					VK_TYPE(VkPipeline) pipeline;
					VK_TYPE(VkPipelineLayout) layout;
				} lightCulling;
				struct {
					VK_TYPE(VkPipeline) pipeline;
					VK_TYPE(VkPipelineLayout) layout;
				} meshCulling;
			} lateCull;  // compute
			struct {
				struct {
					VK_TYPE(VkPipeline) pipeline;
					VK_TYPE(VkPipelineLayout) layout;
				} forward;
			} forward;   // graphic
		} pass;

		
		struct { 
			struct { VK_ENUM(VkFormat) format; VK_TYPE(VkSampler) sampler; } depth;
			struct { VK_ENUM(VkFormat) format; } color;
		} attachment;

		struct Descriptor {
			VK_TYPE(VkDescriptorPool) pool;
			VK_TYPE(VkDescriptorSetLayout) layout;
		} descriptor;

		std::vector<Frame> frames;
	};
}