#define ARAWN_IMPLEMENTATION
#include "texture.h"
#include "engine.h"
#include "buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cstring>

Texture::Texture(
    void* data, uint32_t width, uint32_t height, uint32_t mip_levels,
    VkFormat format, 
    VkImageUsageFlags usage, 
    VkImageAspectFlagBits aspect, 
    VkSampleCountFlagBits sample_count,
    VkMemoryPropertyFlags memory_property, 
    std::span<uint32_t> queue_families)
{
    if (data != nullptr) {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    { // create image
        VkImageCreateInfo info{
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr, 0,
            VK_IMAGE_TYPE_2D, format, 
            { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 }, 
            mip_levels, 1, sample_count, VK_IMAGE_TILING_OPTIMAL, usage,
            queue_families.size() < 2 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT, 
            static_cast<uint32_t>(queue_families.size()), queue_families.data(), 
        };
        
        VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &image));
    }

    { // allocate memory
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(engine.device, image, &requirements);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &memory));

        VK_ASSERT(vkBindImageMemory(engine.device, image, memory, 0));   
    }

    { // create view
        VkImageViewCreateInfo info{
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr, 0, 
            image, VK_IMAGE_VIEW_TYPE_2D, format, 
            {   VK_COMPONENT_SWIZZLE_IDENTITY,  
                VK_COMPONENT_SWIZZLE_IDENTITY, 
                VK_COMPONENT_SWIZZLE_IDENTITY, 
                VK_COMPONENT_SWIZZLE_IDENTITY 
            }, 
            { aspect, 0, mip_levels, 0, 1 }
        };

        VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &view));
    }

    if (data == nullptr) return;
    assert(format == VK_FORMAT_R8G8B8A8_UNORM);
    
    uint32_t buffer_size = 4 * width * height;

    Buffer staging(data, buffer_size, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        std::array<uint32_t, 1>() = { engine.graphics.family }
    );

    stbi_image_free(data);
    
    VkCommandBuffer cmd_buffer;
    VkFence cmd_finished;
    
    { // allocate cmd buffer
        VkCommandBufferAllocateInfo info{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, 
            engine.graphics.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            1
        };

        VK_ASSERT(vkAllocateCommandBuffers(engine.device, &info, &cmd_buffer));
    }

    { // create fence
        VkFenceCreateInfo info{ 
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0
        };
        VK_ASSERT(vkCreateFence(engine.device, &info, nullptr, &cmd_finished));
    }

    { // begin command 
        VkCommandBufferBeginInfo info{ 
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr
        };
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer, &info));
    }
    
    { // change layout from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier barrier{
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, 
            0, VK_ACCESS_TRANSFER_WRITE_BIT, 
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, 
            image, { aspect, 0, mip_levels, 0, 1 }
        };
        vkCmdPipelineBarrier(cmd_buffer, 
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
            0, 0, nullptr, 0, nullptr, 1, &barrier
        );
    }

    { // copy buffer to image
        VkBufferImageCopy region{0, 0, 0, 
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }, 
            { 0, 0, 0 }, 
            { width, height, 1 }
        };
        vkCmdCopyBufferToImage(cmd_buffer, staging.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    { // generate mipmaps
        VkImageMemoryBarrier barrier{
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, 
            0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, 
            { aspect, 0, 1, 0, 1 }
        };
        int32_t mip_width = width;
        int32_t mip_height = height;

        for (uint32_t i = 1; i < mip_levels; i++) {
            { // change layout from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(cmd_buffer, 
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
                    0, 0, nullptr, 0, nullptr, 1, &barrier
                );
            }

            { // blit mip map
                VkImageBlit blit{
                    { aspect, i - 1, 0, 1 }, 
                    { { 0, 0, 0 }, { mip_width, mip_height, 1 } }, 
                    { aspect, i, 0, 1 }, 
                    { { 0, 0, 0 }, { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 } }, 
                };

                vkCmdBlitImage(cmd_buffer,
                    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit, VK_FILTER_LINEAR
                );
            }

            { // change layout from VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(cmd_buffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                    0, 0, nullptr, 0, nullptr, 1, &barrier
                );
            }

            if (mip_width > 1) mip_width /= 2;
            if (mip_height > 1) mip_height /= 2;
        }
    }

    { // end cmd buffer
        VK_ASSERT(vkEndCommandBuffer(cmd_buffer));

        VkSubmitInfo info{ 
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 
            0, nullptr, nullptr, 1, &cmd_buffer, 0, nullptr 
        };
        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, cmd_finished));
    }

    { // destroy temporary
        vkWaitForFences(engine.device, 1, &cmd_finished, VK_TRUE, UINT64_MAX);

        vkFreeCommandBuffers(engine.device, engine.graphics.pool, 1, &cmd_buffer);
        vkDestroyFence(engine.device, cmd_finished, nullptr);
    }
}

Texture::Texture(std::filesystem::path fp, 
    VkImageUsageFlags usage, 
    VkImageAspectFlagBits aspect, 
    VkSampleCountFlagBits sample_count,
    VkMemoryPropertyFlags memory_property, 
    std::span<uint32_t> queue_families
) {
    int width, height, channels;

    if (!fp.has_extension()) {
        image = nullptr;
        return;
    }

    void* data = stbi_load(fp.string().c_str(), &width, &height,  &channels, 4);

    if (width == 0 || height == 0) {
        throw std::runtime_error("failed to load texture image");
    }

    uint32_t mip_levels = std::min<uint32_t>(MAX_MIPMAP_LEVEL, static_cast<uint32_t>(std::log2(std::max(width, height)) + 1));

    *this = Texture(data, width, height, mip_levels, VK_FORMAT_R8G8B8A8_UNORM, 
        usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 
        aspect, sample_count, memory_property, queue_families
    );
}

Texture::Texture(std::filesystem::path fp) : Texture(
    fp, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, std::array<uint32_t, 1>() = { engine.graphics.family }) 
{ }

Texture::~Texture() {
    if (image == nullptr) return;

    vkDestroyImageView(engine.device, view, nullptr);
    vkFreeMemory(engine.device, memory, nullptr);
    vkDestroyImage(engine.device, image, nullptr);

    image = nullptr;
}

Texture::Texture(Texture&& other) {
    if (this == &other) return;

    image = other.image;
    memory = other.memory;
    view = other.view;

    other.image = nullptr;
}

Texture& Texture::operator=(Texture&& other) {
    if (this == &other) return *this;

    if (image != nullptr) {
        vkDestroyImageView(engine.device, view, nullptr);
        vkFreeMemory(engine.device, memory, nullptr);
        vkDestroyImage(engine.device, image, nullptr);
    } 

    image = other.image;
    memory = other.memory;
    view = other.view;

    other.image = nullptr;

    return *this;
}