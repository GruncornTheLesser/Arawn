#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <initializer_list>

namespace arawn::resources {
    struct Image {
        void create(
            const engine::Device& device,
            uint32_t width, uint32_t height,
            uint32_t layers, uint32_t levels,
            VK_ENUM(VkFormat) format,
            VK_ENUM(VkSampleCountFlagBits) samples,
            VK_ENUM(VkImageUsageFlags) usage,
            VK_ENUM(VmaAllocationCreateFlags) flags,
            std::initializer_list<uint32_t> queues);
        
        VK_TYPE(VkImageView) createView(
            const engine::Device& device,
            VK_ENUM(VkFormat) format,
            VK_ENUM(VkImageAspectFlagBits) aspect,
            uint32_t mipLevel = 0, uint32_t mipCount = 1
        ) const;

        void destroy(const engine::Device& device);

        // TODO: create view

        VK_TYPE(VkImage) image;
        VK_TYPE(VmaAllocation) memory;

    };

}