#define ARAWN_INCLUDE_VULKAN
#include <pool/light.h>
#include <core/settings.h>
#include <core/device.h>
#include <node/light.h>

#include <ranges>
#include <algorithm>

using namespace arawn::pool;

void Light::create(const core::Device &device, const core::Settings &settings) {
    std::vector<uint32_t> families = { 
        device.queue.graphics.family,
        device.queue.compute.family,
        device.queue.transfer.family
    };
    std::ranges::sort(families);
    families.erase(std::ranges::unique(families).begin(), families.end());
    
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeof(node::Light) * settings.budget.maxMaterial,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
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
    mapped = reinterpret_cast<node::Light*>(alloc.pMappedData);
}

void Light::destroy(const core::Device &device) {
    vmaDestroyBuffer(device.allocator, buffer, memory);
}

arawn::Handle<arawn::node::Light> Light::create_light(const node::Light& light) {
    Handle<arawn::node::Light> hnd = Pool::create_resource();
    mapped[hnd.index] = light;
    return hnd;
}

void Light::destroy_light(Handle<arawn::node::Light> hnd) {
    Pool::destroy_resource(hnd);
}

void Light::flush() {
    Pool::flush();
}

void Light::flush_all() {
    Pool::flush_all();
}
