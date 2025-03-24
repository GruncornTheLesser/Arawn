#define ARAWN_IMPLEMENTATION
#include "uniform.h"
#include "engine.h"

UniformBuffer<void>::UniformBuffer(uint32_t size) {
    { // create buffer
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.flags = 0;
        info.pNext = nullptr;
        info.pQueueFamilyIndices = &engine.graphics.family;
        info.queueFamilyIndexCount = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.size = size;
        info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VK_ASSERT(vkCreateBuffer(engine.device, &info, nullptr, &buffer));
    }

    { // allocate memory
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(engine.device, buffer, &requirements);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &memory)); 

        VK_ASSERT(vkBindBufferMemory(engine.device, buffer, memory, 0));
    
        VK_ASSERT(vkMapMemory(engine.device, memory, 0, size, 0, &mapped_data));
    }
};

UniformBuffer<void>::~UniformBuffer() {
    if (buffer == nullptr) return;

    vkUnmapMemory(engine.device, memory);
    vkFreeMemory(engine.device, memory, nullptr);
    vkDestroyBuffer(engine.device, buffer, nullptr);
}

UniformBuffer<void>::UniformBuffer(UniformBuffer&& other) {
    if (this == &other) return;

    buffer = other.buffer;
    memory = other.memory;
    mapped_data = other.mapped_data;

    other.buffer = nullptr;
}

UniformBuffer<void>& UniformBuffer<void>::operator=(UniformBuffer&& other) {
    if (this != &other) {
        buffer = other.buffer;
        memory = other.memory;
        mapped_data = other.mapped_data;

        other.buffer = nullptr;
    }

    return *this;
}
