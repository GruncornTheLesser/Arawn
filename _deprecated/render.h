#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <resources/buffer.h>
#include <resources/image.h>
#include <resources/pipeline.h>
#include <vector>

namespace arawn::core {
	struct Render {
		static constexpr uint32_t maxMipLevels = 15;
		
		void create(const Window& window, const Device& device, const World& world, const Swap& Swap, const Settings& info);
		void destroy(const Device& device);
		
		struct Frame {
			VK_TYPE(VkFence) inFlight;

			struct {
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } depthTree; // compute/graphic??
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } earlyCull; // compute: frustum cull, occl cull
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } depthOnly; // graphic: depth prepass
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } lateCull;  // compute: light tile cull, mesh cluster culll
				struct { VK_TYPE(VkCommandBuffer) cmd; VK_TYPE(VkSemaphore) finished; } forward;   // graphic: forward pass
			} pass;

			struct : resources::Buffer {
				primitives::Camera* mapped;
			} camera;
			
			struct { // mesh instances
				resources::Buffer manifest;
				resources::Buffer early;
				resources::Buffer late;
				resources::Buffer cluster;
			} instance;

			struct { 
				resources::Buffer manifest;
				resources::Buffer tile;
			} light;

			struct {
				struct : resources::Image { 
					VK_TYPE(VkImageView) view;
					VK_TYPE(VkImageView) mips[maxMipLevels]; // depth tree
				} depth;
				struct : resources::Image { 
					VK_TYPE(VkImageView) view;
				} color;
			} attachment;

			struct {
				VK_TYPE(VkDescriptorSet) set;
			} descriptor;
		};
		
		struct {
			struct {
				resources::Pipeline generate;
			} depthTree; // compute/graphic??
			struct { 
				resources::Pipeline frustumCull;
				resources::Pipeline occlusionCull; // reprojection cull
			} earlyCull; // compute: frustum cull, occl cull
			struct { 
				resources::Pipeline depthPass; // rdepth prepass

			} depthOnly; // graphic
			
			struct { 
				resources::Pipeline generateTiles;
				resources::Pipeline generateClusters;
				resources::Pipeline lightCulling;
				resources::Pipeline meshCulling;
			} lateCull;  // compute
			struct {
				resources::Pipeline forward;
			} forward;   // graphic
		} pass;

		
		struct { 
			struct { VK_ENUM(VkFormat) format; VK_TYPE(VkSampler) sampler; } depth;
			struct { VK_ENUM(VkFormat) format; } color;
		} attachment;

		struct {
			VK_TYPE(VkDescriptorPool) pool;
			VK_TYPE(VkDescriptorSetLayout) layout;
		} descriptor;

		std::vector<Frame> frames;
		uint32_t index;

		std::vector<VK_TYPE(VkCommandBuffer)> present; // copies color attachment to swap image
	};

}