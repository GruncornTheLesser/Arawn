#pragma once
#include "device.h"
#include <vector>

namespace arawn::engine {
	struct Render {
		void create(const Core& core, const Window& window, const Device& device, const Settings& info);
		void recreate(const Core& core, const Window& window, const Device& device, const Settings& info);
		void destroy(const Core& core, const Device& device);

		struct Frame {
			VK_TYPE(VkCommandBuffer) cmd;
			VK_TYPE(VkFence) inFlight;
			struct {
				struct { VK_TYPE(VkSemaphore) finished; } generateDepthTree;
				struct { VK_TYPE(VkSemaphore) finished; } occlusionCulling;
				struct { VK_TYPE(VkSemaphore) finished; } depthPrepass;
				struct { VK_TYPE(VkSemaphore) finished; } generateClusters;
				struct { VK_TYPE(VkSemaphore) finished; } lightCulling;
				struct { VK_TYPE(VkSemaphore) finished; } meshCulling;
				struct { VK_TYPE(VkSemaphore) finished; } earlyForward;
				struct { VK_TYPE(VkSemaphore) finished; } lateForward;
				struct { VK_TYPE(VkSemaphore) finished; } acquireImage;
				struct { VK_TYPE(VkSemaphore) finished; } render;
			} pass;

			struct {
				VK_TYPE(VkBuffer) buffer;
				VK_TYPE(VmaAllocation) memory;
				struct {
					VK_TYPE(VkBuffer) buffer;
					VK_TYPE(VmaAllocation) memory;
				} staging;
				struct {
					VK_TYPE(VkDescriptorSet) set;
				} descriptor;
			} camera;
			
			struct {
				struct {
					VK_TYPE(VkBuffer) buffer;
					VK_TYPE(VmaAllocation) memory;
				} early;
				struct {
					VK_TYPE(VkBuffer) buffer;
					VK_TYPE(VmaAllocation) memory;
				} late;
				struct {
					VK_TYPE(VkBuffer) buffer;
					VK_TYPE(VmaAllocation) memory;
				} manifest;
			} mesh;

			struct { 
				VK_TYPE(VkBuffer) buffer;
				VK_TYPE(VmaAllocation) memory;
			} light;

			struct { // bounding boxes per tile
				VK_TYPE(VkBuffer) buffer;
				VK_TYPE(VmaAllocation) memory;
			} cluster;

			struct { 
				struct { 
					VK_TYPE(VkImage) image;
					VK_TYPE(VmaAllocation) memory;
					struct {
						VK_TYPE(VkImageView) standard;
						VK_TYPE(VkImageView) tree;
						VK_TYPE(VkImageView) mips[15];
					} view;
				} depth;

				struct { 
					VK_TYPE(VkImage) image;
					VK_TYPE(VmaAllocation) memory;
					struct {
						VK_TYPE(VkImageView) standard;
					} view;
				} color;
			} attachment;

			struct {
				VK_TYPE(VkDescriptorSet) set;
			} descriptor;
		};

		struct {
			struct {
				VK_TYPE(VkPipeline) pipeline;
				VK_TYPE(VkPipelineLayout) layout;
			} generateDepthTree;
			struct {
				VK_TYPE(VkPipeline) pipeline;
				VK_TYPE(VkPipelineLayout) layout;
			} occlusionCulling;
			struct {
				VK_TYPE(VkPipeline) pipeline;
				VK_TYPE(VkPipelineLayout) layout;
			} depthPrepass;
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
			struct {
				VK_TYPE(VkPipeline) pipeline;
				VK_TYPE(VkPipelineLayout) layout;
			} earlyForward;
			struct {
				VK_TYPE(VkPipeline) pipeline;
				VK_TYPE(VkPipelineLayout) layout;
			} lateForward;
		} pass;

		struct { 
			struct { VK_ENUM(VkFormat) format; } depth;
			struct { VK_ENUM(VkFormat) format; } color;
		} attachment;

		struct {
			VK_TYPE(VkDescriptorSetLayout) layout;
		} descriptor;

		std::vector<Frame> frames;
	};
}