#define ARAWN_IMPLEMENTATION
#include "buffer.h"
#include "engine.h"
#include <ranges>
#include <algorithm>

Buffer::Buffer( 
        const void* data, uint32_t size, 
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags memory_properties,
        std::span<uint32_t> queue_families
) {
    { // create buffer
        auto unique_queue_families = std::ranges::unique(queue_families);

        VkBufferCreateInfo info{
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0, 
            size, usage,  
            unique_queue_families.size() < 2 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
            static_cast<uint32_t>(unique_queue_families.size()), unique_queue_families.data()
        };

        VK_ASSERT(vkCreateBuffer(engine.device, &info, nullptr, &buffer));
    }

    { // allocate memory
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(engine.device, buffer, &requirements);

        VkMemoryAllocateInfo info{
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, 
            requirements.size, 
            engine.memory_type_index(requirements, memory_properties)
        };

        VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &memory)); 

        VK_ASSERT(vkBindBufferMemory(engine.device, buffer, memory, 0));
    }

    if (data != nullptr) set_value(data, size);
};

Buffer::~Buffer() {
    if (buffer == nullptr) return;

    vkFreeMemory(engine.device, memory, nullptr);
    vkDestroyBuffer(engine.device, buffer, nullptr);
}

Buffer::Buffer(Buffer&& other) {
    if (this == &other) return;

    buffer = other.buffer;
    memory = other.memory;
    other.buffer = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) {
    if (this == &other) return *this;

    if (buffer != nullptr) {
        vkFreeMemory(engine.device, memory, nullptr);
        vkDestroyBuffer(engine.device, buffer, nullptr);
    }

    buffer = other.buffer;
    memory = other.memory;
    other.buffer = nullptr;

    return *this;
}

void Buffer::set_value(const void* data, uint32_t size, uint32_t offset) {
    void* mapped_data;
    VK_ASSERT(vkMapMemory(engine.device, memory, offset, size, 0, &mapped_data));
    std::memcpy(mapped_data, data, size);
    vkUnmapMemory(engine.device, memory);
}
