#define ARAWN_INCLUDE_VULKAN
#include <pool/node.h>
#include <core/settings.h>
#include <core/device.h>
#include <node/node.h>

#include <ranges>
#include <algorithm>

using namespace arawn::pool;

void Node::create(const core::Device &device, const core::Settings &settings) {
    std::vector<uint32_t> families = { device.queue.graphics.family, device.queue.transfer.family };
    std::ranges::sort(families);
    families.erase(std::ranges::unique(families).begin(), families.end());
    
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeof(node::Node) * settings.budget.maxMaterial,
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
    mapped = reinterpret_cast<node::Node*>(alloc.pMappedData);
}

void Node::destroy(const core::Device &device) {
    vmaDestroyBuffer(device.allocator, buffer, memory);
}

arawn::Handle<arawn::node::Node> Node::create_node(const node::Node& node) {
    Handle<node::Node> hnd = Pool::create_resource();
    mapped[hnd.index] = node;
    return hnd;
}

void Node::destroy_node(Handle<arawn::node::Node> hnd) {
    Pool::destroy_resource(hnd);
}

void Node::flush() {
    Pool::flush();
}

void Node::flush_all() {
    Pool::flush_all();
}
