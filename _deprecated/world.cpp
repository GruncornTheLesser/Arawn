#define ARAWN_INCLUDE_VULKAN
#include <engine/world.h>
#include <engine/device.h>
#include <common/settings.h>
#include <primitive/mesh.h>
#include <primitive/material.h>
#include <primitive/instance.h>

using namespace arawn;

void engine::World::create(const Device& device, const Settings& settings) {
	mesh.pool.create(device, settings.budget.maxMeshes * sizeof(primitive::Mesh), 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		0, 
		{ device.queue.compute.family, device.queue.graphics.family, device.queue.transfer.family }
	);
	
	mesh.arena.create(device, settings.budget.allocation, 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		{ device.queue.compute.family, device.queue.graphics.family, device.queue.transfer.family }
	);
	mesh.arena.block = mesh.arena.createBlock(settings.budget.allocation);

	material.create(device, settings.budget.maxMaterial * sizeof(primitive::Material), 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		0, 
		{ device.queue.compute.family, device.queue.graphics.family, device.queue.transfer.family }
	);

	texture.create(device, 
		settings.texture.capacity.width * settings.texture.resolution.width, settings.texture.capacity.height * settings.texture.resolution.height,
		1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, 
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		{ device.queue.compute.family, device.queue.graphics.family, device.queue.transfer.family }
	);

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

		VK_ASSERT(vkCreateSampler(device.device, &samplerInfo, nullptr, &texture.sampler));
	}

	instance.create(device, settings.budget.maxInstances * sizeof(primitive::Instance), 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 
		{ device.queue.compute.family, device.queue.graphics.family, device.queue.transfer.family }
	);

	{ // world descriptor
		auto bindings = std::to_array<VkDescriptorSetLayoutBinding>({
			{ // mesh buffer
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			{ // mesh arena
				.binding = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			{ // material buffer
				.binding = 2,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			{ // material buffer
				.binding = 3,
				.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			{ // instance buffer
				.binding = 4,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			{ // light buffer
				.binding = 5,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				.pImmutableSamplers = nullptr,
			},
			{ // uniform buffer
				.binding = 6,
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
	vkDestroyDescriptorSetLayout(device.device, descriptor.layout, nullptr);	
	
	instance.destroy(device);
	
	vkDestroySampler(device.device, texture.sampler, nullptr);
	texture.destroy(device); 
	material.destroy(device);
	
	mesh.pool.destroy(device);
	
	vmaDestroyVirtualBlock(mesh.arena.block);
	mesh.arena.destroy(device);
}