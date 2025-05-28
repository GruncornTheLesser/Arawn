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
        
        auto unique_queue_families = std::ranges::subrange(queue_families.begin(), std::ranges::unique(queue_families).begin());

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

    if (data == nullptr) return;

    if (memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        set_value(data, size);
        return;
    }
    
    if (usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) {
        Buffer staging(
            data, size, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            std::array<uint32_t, 1>() = { engine.graphics.family }
        );
        
        VkFence fence;
        VkCommandBuffer cmd_buffer;
        { // allocate command buffer
            VkCommandBufferAllocateInfo info{
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
                engine.graphics.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1
            };
            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &info, &cmd_buffer));
        }

        { // allocate fence
            VkFenceCreateInfo info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
            VK_ASSERT(vkCreateFence(engine.device, &info, nullptr, &fence));
        }

        { // record command buffer
            VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr };
            VK_ASSERT(vkBeginCommandBuffer(cmd_buffer, &begin_info));

            VkBufferCopy copy_region{0, 0, size};
            vkCmdCopyBuffer(cmd_buffer, staging.buffer, buffer, 1, &copy_region);
            
            VK_ASSERT(vkEndCommandBuffer(cmd_buffer));
        }

        { // submit command buffer
            VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0 };
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmd_buffer;

            VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &submit_info, fence));
            VK_ASSERT(vkQueueWaitIdle(engine.graphics.queue));
        }

        { // clean up
            VK_ASSERT(vkWaitForFences(engine.device, 1, &fence, VK_TRUE, UINT64_MAX));

            vkFreeCommandBuffers(engine.device, engine.graphics.pool, 1, &cmd_buffer);
            vkDestroyFence(engine.device, fence, nullptr);
        }

        return;
    }

    throw std::runtime_error("cannot initialize buffer data");
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
