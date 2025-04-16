#define ARAWN_IMPLEMENTATION
#include "uniform.h"
#include "engine.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <cstring>

UniformBuffer::UniformBuffer(const void* data, uint32_t size) {
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
    }

    if (data != nullptr) set_value(data, size);
};

UniformBuffer::~UniformBuffer() {
    if (buffer == nullptr) return;

    vkFreeMemory(engine.device, memory, nullptr);
    vkDestroyBuffer(engine.device, buffer, nullptr);
}

UniformBuffer::UniformBuffer(UniformBuffer&& other) {
    if (this == &other) return;

    buffer = other.buffer;
    memory = other.memory;

    other.buffer = nullptr;
}

UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) {
    std::swap(buffer, other.buffer);
    std::swap(memory, other.memory);

    return *this;
}

void UniformBuffer::set_value(const void* data, uint32_t size) {
    void* mapped_data;
    VK_ASSERT(vkMapMemory(engine.device, memory, 0, size, 0, &mapped_data));
    std::memcpy(mapped_data, data, size);
    vkUnmapMemory(engine.device, memory);
}



UniformTexture::UniformTexture(std::filesystem::path fp) {
    int width, height, channels;

    if (!fp.has_extension()) {
        image = nullptr;
        return;
    }
    
    void* data = stbi_load(fp.string().c_str(), &width, &height,  &channels, 4);

    if (width == 0 || height == 0) {
        throw std::runtime_error("failed to load texture image");
    }

    VkDeviceSize buffer_size = width * height * 4;
    uint32_t mip_levels = std::min<uint32_t>(MAX_MIPMAP_LEVEL, static_cast<uint32_t>(std::log2(std::max(width, height)) + 1));

    { // create image
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        info.mipLevels = mip_levels;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.pQueueFamilyIndices = &engine.graphics.family;
        info.queueFamilyIndexCount = 1;
        
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
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.image = image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = mip_levels;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;

        VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &view));
    }

    VkCommandBuffer cmd_buffer;
    VkFence finished;
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;

    { // allocate temporary cmd buffer
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandPool = engine.graphics.pool;
        info.commandBufferCount = 1;

        VK_ASSERT(vkAllocateCommandBuffers(engine.device, &info, &cmd_buffer));
    }

    { // create fence
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;

        VK_ASSERT(vkCreateFence(engine.device, &info, nullptr, &finished));
    }

    { // create temporary staging buffer
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.flags = 0;
        info.pNext = nullptr;
        info.pQueueFamilyIndices = &engine.graphics.family;
        info.queueFamilyIndexCount = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.size = buffer_size;
        info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VK_ASSERT(vkCreateBuffer(engine.device, &info, nullptr, &staging_buffer));
    }
    
    { // allocate temporary staging memory
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(engine.device, staging_buffer, &requirements);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &staging_memory)); 

        VK_ASSERT(vkBindBufferMemory(engine.device, staging_buffer, staging_memory, 0));
    }
    
    { // write data to staging memory
        void* buffer_data;

        VK_ASSERT(vkMapMemory(engine.device, staging_memory, 0, buffer_size, 0 , &buffer_data));
        memcpy(buffer_data, data, buffer_size);
        vkUnmapMemory(engine.device, staging_memory);

        stbi_image_free(data);
    }

    { // begin command 
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer, &info));
    }
    
    { // change layout from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier barrier{};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mip_levels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        vkCmdPipelineBarrier(cmd_buffer, 
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
            VK_PIPELINE_STAGE_TRANSFER_BIT, 
            0, // dependency_flags
            0, nullptr, // memory barriers
            0, nullptr, // buffer memory barrier count
            1, &barrier
        );

        VkSemaphoreWaitInfo info{};
    }

    { // copy buffer to image
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent.width = width;
        region.imageExtent.height = height;
        region.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(cmd_buffer, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    { // generate mipmaps
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

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
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
            }

            { // blit mip map
                VkImageBlit blit{};
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = {mip_width, mip_height, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(cmd_buffer,
                    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR
                );
            }

            { // change layout from VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(cmd_buffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
            }

            if (mip_width > 1) mip_width /= 2;
            if (mip_height > 1) mip_height /= 2;
        }
    }

    { // end cmd buffer
        VK_ASSERT(vkEndCommandBuffer(cmd_buffer));

        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &cmd_buffer;

        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, finished));
    }

    { // destroy temporary
        vkWaitForFences(engine.device, 1, &finished, VK_TRUE, UINT64_MAX);

        vkFreeCommandBuffers(engine.device, engine.graphics.pool, 1, &cmd_buffer);
        vkDestroyBuffer(engine.device, staging_buffer, nullptr);
        vkFreeMemory(engine.device, staging_memory, nullptr);
        vkDestroyFence(engine.device, finished, nullptr);        
    }
}

UniformTexture::~UniformTexture() {
    if (image == nullptr) return;

    vkDestroyImageView(engine.device, view, nullptr);
    vkFreeMemory(engine.device, memory, nullptr);
    vkDestroyImage(engine.device, image, nullptr);

    image = nullptr;
}

UniformTexture::UniformTexture(UniformTexture&& other) {
    if (this == &other) return;

    image = other.image;
    memory = other.memory;
    view = other.view;

    other.image = nullptr;
}

UniformTexture& UniformTexture::operator=(UniformTexture&& other) {
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

UniformSet::UniformSet(VkDescriptorSetLayout layout, std::span<Uniform> bindings) {
    { // allocate descriptor set
        VkDescriptorSetAllocateInfo info{};
        info.pNext = nullptr;
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        
        info.descriptorSetCount = 1;
        info.descriptorPool = engine.descriptor_pool;
        info.pSetLayouts = &layout;
        
        VK_ASSERT(vkAllocateDescriptorSets(engine.device, &info, &descriptor_set));
    }

    { // write descriptor set
        std::vector<VkWriteDescriptorSet> write_infos;
        std::vector<VkDescriptorBufferInfo> buffer_infos;
        std::vector<VkDescriptorImageInfo> image_infos;

        for (uint32_t i = 0; i < bindings.size(); ++i) {
            if (std::holds_alternative<UniformBuffer*>(bindings[i])) {
                UniformBuffer& uniform_buffer = *std::get<UniformBuffer*>(bindings[i]);

                if (uniform_buffer.buffer != nullptr) {
                    auto& buffer_info = buffer_infos.emplace_back();
                    buffer_info.buffer = uniform_buffer.buffer;
                    buffer_info.offset = 0;
                    buffer_info.range = VK_WHOLE_SIZE;

                    auto& write_info = write_infos.emplace_back();
                    write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write_info.pNext = nullptr;
                    write_info.dstSet = descriptor_set;
                    write_info.dstBinding = i;
                    write_info.dstArrayElement = 0;
                    write_info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    write_info.descriptorCount = 1;
                    write_info.pBufferInfo = &buffer_info;
                }
            } else { // std::holds_alternative<UniformTexture*>(bindings[i])
                UniformTexture& uniform_texture = *std::get<UniformTexture*>(bindings[i]);

                if (uniform_texture.image != nullptr) {
                    auto& image_info = image_infos.emplace_back();
                    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    image_info.sampler = engine.sampler;
                    image_info.imageView = uniform_texture.view;

                    auto& write_info = write_infos.emplace_back();
                    write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write_info.pNext = nullptr;
                    write_info.dstSet = descriptor_set;
                    write_info.dstBinding = i;
                    write_info.dstArrayElement = 0;
                    write_info.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    write_info.descriptorCount = 1;
                    write_info.pImageInfo = &image_info;
                    write_info.pBufferInfo = nullptr;
                }
            }
        }

        vkUpdateDescriptorSets(engine.device, write_infos.size(), write_infos.data(), 0, nullptr);
    }
}

UniformSet::~UniformSet() {
    if (descriptor_set == nullptr) return;
    
    vkFreeDescriptorSets(engine.device, engine.descriptor_pool, 1, &descriptor_set);
    descriptor_set = nullptr;
}

UniformSet::UniformSet(UniformSet&& other) {
    if (this == &other) return;

    descriptor_set = other.descriptor_set;
    other.descriptor_set = nullptr;
}

UniformSet& UniformSet::operator=(UniformSet&& other) {
    if (this == &other) return *this;

    if (descriptor_set != nullptr) {
        vkFreeDescriptorSets(engine.device, engine.descriptor_pool, 1, &descriptor_set);
    }

    descriptor_set = other.descriptor_set;
    other.descriptor_set = nullptr;

    return *this;
}