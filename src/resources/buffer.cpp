#define ARAWN_INCLUDE_VULKAN
#include <resources/buffer.h>
#include <engine/device.h>
#include <vector>
#include <ranges>
#include <algorithm>

using namespace arawn::resources;

void Buffer::create(const engine::Device &device,
    std::size_t size,
    VkBufferUsageFlags usage,
    VmaAllocationCreateFlags flags,
    std::initializer_list<uint32_t> queues,
    void **mapped
) {
    std::vector<uint32_t> families(queues);
    std::ranges::sort(families);
    families.erase(std::ranges::unique(families).begin(), families.end());

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
        .pQueueFamilyIndices = families.data(),
    };

    VmaAllocationCreateInfo allocInfo{
        .flags = flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    
    VmaAllocationInfo info{};
    VK_ASSERT(vmaCreateBuffer(device.allocator, &bufferInfo, &allocInfo, &buffer, &memory, &info));
    if (mapped != nullptr) *mapped = info.pMappedData;
}

void Buffer::destroy(const engine::Device &device) {
    vmaDestroyBuffer(device.allocator, buffer, memory);
}