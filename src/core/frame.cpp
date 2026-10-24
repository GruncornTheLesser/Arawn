#define ARAWN_INCLUDE_VULKAN
#include <core/frame.h>
#include <core/device.h>
#include <core/settings.h>

using namespace arawn::core;

void Frame::create(const core::Device& device, const core::Settings &settings) {
    index = 0;
    count = settings.buffering == BufferingMode::TRIPLE ? 3 : 2;

    VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    for (uint32_t i = 0; i < count; ++i) {
        VK_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &inFlight[i]));
    }

    VkSemaphoreCreateInfo semaInfo{ 
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };
    for (uint32_t i = 0; i < count; ++i) {
        VK_ASSERT(vkCreateSemaphore(device, &semaInfo, nullptr, &imageReady[i]));
    }

    if (count != ARAWN_MAX_FRAMES) {
        inFlight[count] = nullptr;
        imageReady[count] = nullptr;
    }
}

void Frame::destroy(const core::Device& device) {
    for (uint32_t i = 0; i < count; ++i) {
        vkDestroySemaphore(device, imageReady[i], nullptr);
        vkDestroyFence(device, inFlight[i], nullptr);
    }
}

void Frame::advance() {
    index = (index + 1) % count;
}
