#define ARAWN_INCLUDE_VULKAN
#include <pool/material.h>
#include <core/settings.h>
#include <core/device.h>
#include <asset/material.h>
#include <primitive/material.h>

#include <ranges>
#include <algorithm>

using namespace arawn::pool;

void Material::create(const core::Device &device, const core::Settings &settings) {
    std::vector<uint32_t> families = { device.queue.graphics.family, device.queue.transfer.family };
    std::ranges::sort(families);
    families.erase(std::ranges::unique(families).begin(), families.end());
    
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeof(primitive::Material) * settings.budget.maxMaterial,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT ,
        .sharingMode = families.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount =  static_cast<uint32_t>(families.size()),
        .pQueueFamilyIndices =  families.data(),
    };

    VmaAllocationCreateInfo allocInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    VmaAllocationInfo alloc;
    VK_ASSERT(vmaCreateBuffer(device.allocator, &bufferInfo, &allocInfo, &buffer, &memory, &alloc));
    mapped = reinterpret_cast<primitive::Material*>(alloc.pMappedData);
}

void Material::destroy(const core::Device &device) {
    vmaDestroyBuffer(device.allocator, buffer, memory);
}

arawn::Handle<arawn::asset::Material> Material::create_material(const asset::Material& material) {
    Handle<arawn::asset::Material> hnd = pool.create();
    primitive::Material& primitive = mapped[hnd.index];
    primitive = {
        .albedo = material.albedo,
		.roughness = material.roughness,
		.metallic = material.metallic,
		.albedoMap = material.albedoMap.index,
		.normalMap = material.normalMap.index,
		.metalRoughMap = material.metalRoughMap.index,
    };
    return hnd;
}

void Material::destroy_material(Handle<arawn::asset::Material> hnd) {
    pool.destroy(hnd);
}