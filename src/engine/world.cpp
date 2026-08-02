#define ARAWN_INCLUDE_VULKAN
#include <engine/world.h>
#include <assets/mesh.h>
#include <nodes/instance.h>
#include <algorithm>
#include <ranges>
using namespace arawn;

void engine::World::create(const Device& device, const Settings& info) {
	std::array requiredFamilies = std::to_array({ 
		device.queue.compute.family, 
		device.queue.graphics.family,
	});
	std::ranges::sort(requiredFamilies);
	std::span<uint32_t> families = { 
		requiredFamilies.data(),
		static_cast<uint32_t>(std::ranges::unique(requiredFamilies).begin() - requiredFamilies.begin())
	};
	VkSharingMode sharingMode = families.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	// ---------- mesh data ----------
	{ // mesh buffer
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = info.geometry.meshCount * sizeof(primitives::Mesh),
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.sharingMode = sharingMode,
			.queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
			.pQueueFamilyIndices = families.data(),
		};
		VmaAllocationCreateInfo allocInfo{
			.flags = 0,
			.usage = VMA_MEMORY_USAGE_GPU_ONLY
		};
		
		VK_ASSERT(vmaCreateBuffer(device.allocator, &bufferInfo, &allocInfo, &mesh.buffer, &mesh.memory, nullptr));
	}

	{ // mesh arena
		VkBufferCreateInfo bufferInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = info.geometry.budget,
			.usage =
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | 
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
				VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
			.pQueueFamilyIndices = families.data(),
		};
	
		VmaAllocationCreateInfo allocInfo {
			.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO,
		};
	
		VmaVirtualBlockCreateInfo blockInfo{ 
			.size = info.geometry.budget
		};
		
		VK_ASSERT(vmaCreateBuffer(device.allocator, &bufferInfo, &allocInfo, &mesh.arena.buffer, &mesh.arena.memory, nullptr));
		VK_ASSERT(vmaCreateVirtualBlock(&blockInfo, &mesh.arena.block));
	}

	// ---------- material data ----------
	{ // material buffer
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = info.materials.capacity * sizeof(primitives::Mesh),
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.sharingMode = sharingMode,
			.queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
			.pQueueFamilyIndices = families.data(),
		};
		VmaAllocationCreateInfo allocInfo{
			.flags = 0,
			.usage = VMA_MEMORY_USAGE_GPU_ONLY
		};
		
		VK_ASSERT(vmaCreateBuffer(device.allocator, &bufferInfo, &allocInfo, &material.buffer, &material.memory, nullptr));
	}

	{ // texture
		VkImageCreateInfo imageInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.extent = {
				info.texture.capacity.width * info.texture.resolution.width,
				info.texture.capacity.height * info.texture.resolution.height,
				1
			},
			.mipLevels = info.texture.miplevels,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT | 
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
				VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.sharingMode = (families.size() > 1) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
			.pQueueFamilyIndices = families.data(),
		};
	
		VmaAllocationCreateInfo allocInfo = {
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		};
		
		VK_ASSERT(vmaCreateImage(device.allocator, &imageInfo, &allocInfo, &material.texture.image, &material.texture.memory, nullptr));
	}

	{ // sampler
		VkSamplerCreateInfo samplerInfo{

			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_FALSE,
			.maxAnisotropy = 0,
			.compareEnable = VK_FALSE,
			// .compareOp = ,
			.minLod = 0.0f, // mipmapping field
			.maxLod = 0.0f,
			// .borderColor = ,
			.unnormalizedCoordinates = VK_FALSE,
		};

		VK_ASSERT(vkCreateSampler(device.device, &samplerInfo, nullptr, &material.texture.sampler));
	}

	// ---------- instance data ----------
	{ // instance buffer
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = info.geometry.instanceCount * sizeof(primitives::Instance),
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.sharingMode = sharingMode,
			.queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
			.pQueueFamilyIndices = families.data(),
		};
		VmaAllocationCreateInfo allocInfo{
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		};
		
		VK_ASSERT(vmaCreateBuffer(device.allocator, &bufferInfo, &allocInfo, &instance.buffer, &instance.memory, nullptr));
	}

	{ // descriptor
		auto bindings = std::to_array({
			VkDescriptorSetLayoutBinding{ // mesh buffer
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			VkDescriptorSetLayoutBinding{ // mesh arena
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			VkDescriptorSetLayoutBinding{ // material buffer
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			VkDescriptorSetLayoutBinding{ // material buffer
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			VkDescriptorSetLayoutBinding{ // instance buffer
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			VkDescriptorSetLayoutBinding{ // light buffer
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			VkDescriptorSetLayoutBinding{ // uniform buffer
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
		});
		VkDescriptorSetLayoutCreateInfo layoutInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data(),
		};

		VK_ASSERT(vkCreateDescriptorSetLayout(device.device, &layoutInfo, nullptr, &descriptor.layout));
	
		VkDescriptorSetAllocateInfo descriptorInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = device.descriptor.pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &descriptor.layout,
		};

		VK_ASSERT(vkAllocateDescriptorSets(device.device, &descriptorInfo, &descriptor.set));
	}
}

void engine::World::destroy(const Device& device) {
	// ---------- descriptor data ----------
	vkFreeDescriptorSets(device.device, device.descriptor.pool, 1, &descriptor.set);
	vkDestroyDescriptorSetLayout(device.device, descriptor.layout, nullptr);
	
	// ---------- instance data ----------
	vmaDestroyBuffer(device.allocator, instance.buffer, nullptr);

	// ---------- material data ----------
	vkDestroySampler(device.device, material.texture.sampler, nullptr);
	vmaDestroyImage(device.allocator, material.texture.image, nullptr);;
	vmaDestroyBuffer(device.allocator, material.buffer, nullptr);
	
	// ---------- mesh data ----------
	vmaDestroyBuffer(device.allocator, mesh.buffer, nullptr);
	vmaDestroyVirtualBlock(mesh.arena.block);
	vmaDestroyBuffer(device.allocator, mesh.arena.buffer, nullptr);	
}