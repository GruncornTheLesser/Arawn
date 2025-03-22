#define ARAWN_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "material.h"
#include "engine.h"
#include "renderer.h"



Material::Material() {
    tinyobj::material_t default_material;
    default_material.diffuse[0] = 0.0f;
    default_material.diffuse[1] = 0.0f;
    default_material.diffuse[2] = 0.0f;
    default_material.diffuse_texname = "res/images/default.png";
    default_material.normal_texname = "res/images/default.png";
    default_material.roughness = 0.0f;
    default_material.roughness_texname = "";
    default_material.metallic = 0.0f;
    default_material.metallic_texname = "";
    default_material.normal_texname = "";

    *this = Material(&default_material);
}

Material::Material(const tinyobj::material_t* info) {
    { // create buffer
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.flags = 0;
        info.pNext = nullptr;
        info.pQueueFamilyIndices = &engine.graphics.family;
        info.queueFamilyIndexCount = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.size = sizeof(Material::Data);
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

    vkMapMemory(engine.device, memory, 0, sizeof(Data), 0, reinterpret_cast<void**>(&mapped_data));
    
    mapped_data->albedo = { info->diffuse[0], info->diffuse[1], info->diffuse[2] };
    mapped_data->metallic = info->metallic;
    mapped_data->roughness = info->roughness;

    if (!info->diffuse_texname.empty()) {
        mapped_data->flags |= Material::ALBEDO_TEXTURE;
        albedo_texture = { info->diffuse_texname, VK_FORMAT_R8G8B8A8_UNORM };
    }

    if (!info->metallic_texname.empty()) {
        mapped_data->flags |= Material::METALLIC_TEXTURE;
        metallic_texture = { info->metallic_texname, VK_FORMAT_R8_UNORM };
    }

    if (!info->roughness_texname.empty()) {
        mapped_data->flags |= Material::ROUGHNESS_TEXTURE;
        roughness_texture = { info->roughness_texname, VK_FORMAT_R8_UNORM };
    }

    if (!info->normal_texname.empty()) {
        mapped_data->flags |= Material::NORMAL_TEXTURE;
        normal_texture = { info->normal_texname, VK_FORMAT_R8G8B8_UNORM };
    }

    { // allocate descriptor set
        VkDescriptorSetAllocateInfo info{};
        info.pNext = nullptr;
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorSetCount = 1;
        info.descriptorPool = engine.descriptor_pool;
        info.pSetLayouts = &engine.material_layout;
        
        VK_ASSERT(vkAllocateDescriptorSets(engine.device, &info, &set));
    }

    { // bind descriptor set
        VkWriteDescriptorSet desc_writes[5];
        VkDescriptorBufferInfo buffer_info[1];
        VkDescriptorImageInfo image_info[4];

        uint32_t desc_count = 0;
        uint32_t buffer_count = 0;
        uint32_t image_count = 0;
        {
            auto& uniform_write = buffer_info[buffer_count++];
            uniform_write.buffer = buffer;
            uniform_write.offset = 0;
            uniform_write.range = sizeof(Data);

            auto& uniform_desc = desc_writes[desc_count++];
            uniform_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uniform_desc.pNext = nullptr;
            uniform_desc.dstSet = set;
            uniform_desc.dstBinding = 0;
            uniform_desc.dstArrayElement = 0;
            uniform_desc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniform_desc.descriptorCount = 1;
            uniform_desc.pBufferInfo = &uniform_write;
        }

        if (albedo_texture.image != nullptr) {
            auto& albedo_write = image_info[image_count++];
            albedo_write.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            albedo_write.imageView = albedo_texture.view;
            albedo_write.sampler = engine.sampler;

            auto& albedo_desc = desc_writes[desc_count++];
            albedo_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            albedo_desc.pNext = nullptr;
            albedo_desc.dstSet = set;
            albedo_desc.dstBinding = 1;
            albedo_desc.dstArrayElement = 0;
            albedo_desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            albedo_desc.descriptorCount = 1;
            albedo_desc.pImageInfo = &albedo_write;
            albedo_desc.pBufferInfo = nullptr;
        }

        if (metallic_texture.image != nullptr) {
            auto& metallic_write = image_info[image_count++];
            metallic_write.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            metallic_write.imageView = metallic_texture.view;
            metallic_write.sampler = engine.sampler;

            auto& metallic_desc = desc_writes[desc_count++];
            metallic_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            metallic_desc.pNext = nullptr;
            metallic_desc.dstSet = set;
            metallic_desc.dstBinding = 2;
            metallic_desc.dstArrayElement = 0;
            metallic_desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            metallic_desc.descriptorCount = 1;
            metallic_desc.pImageInfo = &metallic_write;
            metallic_desc.pBufferInfo = nullptr;
        }

        if (roughness_texture.image != nullptr) {
            auto& roughness_write = image_info[image_count++];
            roughness_write.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            roughness_write.imageView = roughness_texture.view;
            roughness_write.sampler = engine.sampler;

            auto& roughness_desc = desc_writes[desc_count++];
            roughness_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            roughness_desc.pNext = nullptr;
            roughness_desc.dstSet = set;
            roughness_desc.dstBinding = 3;
            roughness_desc.dstArrayElement = 0;
            roughness_desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            roughness_desc.descriptorCount = 1;
            roughness_desc.pImageInfo = &roughness_write;
            roughness_desc.pBufferInfo = nullptr;
        }

        if (normal_texture.image != nullptr) {
            auto& normal_write = image_info[image_count++];
            normal_write.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normal_write.imageView = normal_texture.view;
            normal_write.sampler = engine.sampler;

            auto& normal_desc = desc_writes[desc_count++];
            normal_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            normal_desc.pNext = nullptr;
            normal_desc.dstSet = set;
            normal_desc.dstBinding = 4;
            normal_desc.dstArrayElement = 0;
            normal_desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            normal_desc.descriptorCount = 1;
            normal_desc.pImageInfo = &normal_write;
            normal_desc.pBufferInfo = nullptr;
        }

        vkUpdateDescriptorSets(engine.device, desc_count, desc_writes, 0, nullptr);
    }
}

Material::~Material() {
    if (buffer == nullptr) return;

    vkFreeMemory(engine.device, memory, nullptr);
    vkDestroyBuffer(engine.device, buffer, nullptr);
    vkFreeDescriptorSets(engine.device, engine.descriptor_pool, 1, &set);
}

Material::Material(Material&& other) {
    if (this == &other) return;

    albedo_texture = std::move(other.albedo_texture);
    metallic_texture = std::move(other.metallic_texture);
    roughness_texture = std::move(other.roughness_texture);
    normal_texture = std::move(other.normal_texture);
    
    set = other.set;
    buffer = other.buffer;
    memory = other.memory;
    mapped_data = other.mapped_data;

    other.buffer = nullptr;
}
Material& Material::operator=(Material&& other) {
    albedo_texture = std::move(other.albedo_texture);
    metallic_texture = std::move(other.metallic_texture);
    roughness_texture = std::move(other.roughness_texture);
    normal_texture = std::move(other.normal_texture);

    std::swap(set, other.set);
    std::swap(buffer, other.buffer);
    std::swap(memory, other.memory);
    std::swap(mapped_data, other.mapped_data);

    return *this;
}