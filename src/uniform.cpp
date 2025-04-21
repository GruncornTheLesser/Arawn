#define ARAWN_IMPLEMENTATION
#include "uniform.h"
#include "engine.h"

UniformSet::UniformSet(VkDescriptorSetLayout layout, std::span<Uniform> bindings) {
    { // allocate descriptor set
        VkDescriptorSetAllocateInfo info{ 
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, 
            engine.descriptor_pool, 1, &layout
        };
        
        VK_ASSERT(vkAllocateDescriptorSets(engine.device, &info, &descriptor_set));
    }

    { // write descriptor set
        std::vector<VkWriteDescriptorSet> write_infos(bindings.size());
        std::vector<VkDescriptorBufferInfo> buffer_infos(bindings.size());
        std::vector<VkDescriptorImageInfo> image_infos(bindings.size());
        uint32_t write_count = 0;
        uint32_t buffer_count = 0;
        uint32_t image_count = 0;
        for (uint32_t i = 0; i < bindings.size(); ++i) {
            if (std::holds_alternative<UniformBuffer>(bindings[i])) {
                Buffer& uniform_buffer = *std::get<UniformBuffer>(bindings[i]).ptr;

                if (uniform_buffer.buffer != nullptr) {
                    auto& buffer_info = buffer_infos[buffer_count++];
                    buffer_info.buffer = uniform_buffer.buffer;
                    buffer_info.offset = 0;
                    buffer_info.range = VK_WHOLE_SIZE;

                    auto& write_info = write_infos[write_count++];
                    write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write_info.pNext = nullptr;
                    write_info.dstSet = descriptor_set;
                    write_info.dstBinding = i;
                    write_info.dstArrayElement = 0;
                    write_info.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    write_info.descriptorCount = 1;
                    write_info.pBufferInfo = &buffer_info;
                }
            } else if (std::holds_alternative<StorageBuffer>(bindings[i])) {
                Buffer& storage_buffer = *std::get<StorageBuffer>(bindings[i]).ptr;

                if (storage_buffer.buffer != nullptr) {
                    auto& buffer_info = buffer_infos[buffer_count++];
                    buffer_info.buffer = storage_buffer.buffer;
                    buffer_info.offset = 0;
                    buffer_info.range = VK_WHOLE_SIZE;

                    auto& write_info = write_infos[write_count++];
                    write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write_info.pNext = nullptr;
                    write_info.dstSet = descriptor_set;
                    write_info.dstBinding = i;
                    write_info.dstArrayElement = 0;
                    write_info.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    write_info.descriptorCount = 1;
                    write_info.pBufferInfo = &buffer_info;
                }
            } else if (std::holds_alternative<UniformTexture>(bindings[i])) {
                Texture& uniform_texture = *std::get<UniformTexture>(bindings[i]).ptr;

                if (uniform_texture.image != nullptr) {
                    auto& image_info = image_infos[image_count++];
                    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    image_info.sampler = engine.sampler;
                    image_info.imageView = uniform_texture.view;

                    auto& write_info = write_infos[write_count++];
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
            } else if (std::holds_alternative<InputAttachment>(bindings[i])) {
                Texture& input_attachment = *std::get<InputAttachment>(bindings[i]).ptr;

                if (input_attachment.image != nullptr) {
                    auto& image_info = image_infos[image_count++];
                    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    image_info.sampler = engine.sampler;
                    image_info.imageView = input_attachment.view;

                    auto& write_info = write_infos[write_count++];
                    write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write_info.pNext = nullptr;
                    write_info.dstSet = descriptor_set;
                    write_info.dstBinding = i;
                    write_info.dstArrayElement = 0;
                    write_info.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                    write_info.descriptorCount = 1;
                    write_info.pImageInfo = &image_info;
                    write_info.pBufferInfo = nullptr;
                }
            } else if (std::holds_alternative<DepthAttachment>(bindings[i])) {
                Texture& input_attachment = *std::get<DepthAttachment>(bindings[i]).ptr;

                if (input_attachment.image != nullptr) {
                    auto& image_info = image_infos[image_count++];
                    image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    image_info.sampler = engine.sampler;
                    image_info.imageView = input_attachment.view;

                    auto& write_info = write_infos[write_count++];
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

        vkUpdateDescriptorSets(engine.device, write_count, write_infos.data(), 0, nullptr);
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