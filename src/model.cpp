#include <cstring>
#define VK_IMPLEMENTATION
#include "model.h"
#include "engine.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <tiny_obj_loader.h>
#include <cstring>

Buffer::Buffer(size_t buffer_size, VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags properties, VkSharingMode sharing_mode)
 : size(buffer_size) {
    { // init buffer
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = buffer_size;
        info.usage = usage;
        info.sharingMode = sharing_mode;
        
        VK_ASSERT(vkCreateBuffer(engine.device, &info, nullptr, &buffer));
    }
    { // init memory
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(engine.device, buffer, &requirements);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = engine.get_memory_index(requirements.memoryTypeBits, properties);

        VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &memory));
    }
    VK_ASSERT(vkBindBufferMemory(engine.device, buffer, memory, 0));
}

Buffer::Buffer(const void* data, size_t buffer_size, VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags properties, VkSharingMode sharing_mode)
 : Buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, properties, sharing_mode) {
    Buffer staging(buffer_size, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        VK_SHARING_MODE_EXCLUSIVE
    );
    
    { // copy data to staging buffer
        void* buffer_data;
        VK_ASSERT(vkMapMemory(engine.device, staging.memory, 0, buffer_size, 0, &buffer_data));
        std::memcpy(buffer_data, data, buffer_size);
        vkUnmapMemory(engine.device, staging.memory);
    }

    VkCommandBuffer cmd_buffer;
    { // create new command buffer
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandPool = engine.graphics.pool;
        info.commandBufferCount = 1;

        vkAllocateCommandBuffers(engine.device, &info, &cmd_buffer);
    }

    { // record copy to cmd_buffer
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmd_buffer, &info);

        { // copy staging to vertex buffer 
            VkBufferCopy copy_region{};
            copy_region.size = buffer_size;
            vkCmdCopyBuffer(cmd_buffer, staging.buffer, buffer, 1, &copy_region);

        }

        vkEndCommandBuffer(cmd_buffer);
    }
    
    {  // submit cmd_buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd_buffer;

        vkQueueSubmit(engine.graphics.queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(engine.graphics.queue);
    }

    vkFreeCommandBuffers(engine.device, engine.graphics.pool, 1, &cmd_buffer);
}

Buffer::~Buffer() {
    if (buffer == nullptr) return;
    vkDestroyBuffer(engine.device, buffer, nullptr);
    vkFreeMemory(engine.device, memory, nullptr);
}

Buffer::Buffer(Buffer&& other) {
    buffer = other.buffer;
    memory = other.memory;
    other.buffer = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) {
    std::swap(buffer, other.buffer);
    std::swap(memory, other.memory);
    return *this;
}

Texture::Texture(std::filesystem::path fp, VkImageUsageFlags usage, uint32_t mipmap, 
    VkSampleCountFlagBits sample_count, VkSharingMode share_mode, VkImageAspectFlags aspect) {
    int width, height, channels;
    VkFormat format;

    stbi_uc* data = stbi_load(fp.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    assert(data);
    
    Buffer staging(data, width * height * channels,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        VK_SHARING_MODE_EXCLUSIVE
    );
    
    stbi_image_free(data);

    switch (channels) {
    case 1: format = VK_FORMAT_R8_UNORM;
    case 2: format = VK_FORMAT_R8G8_UNORM;
    case 3: format = VK_FORMAT_R8G8B8_UNORM;
    case 4: format = VK_FORMAT_R8G8B8A8_UNORM;
    }

    *this = Texture(staging, width, height, format, usage, mipmap, sample_count, share_mode, aspect);
}

Texture::Texture(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, 
    uint32_t mipmap, VkSampleCountFlagBits sample_count, VkSharingMode share_mode, VkImageAspectFlags aspect) {
    { // init image
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.extent = { width, height, 1 };
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.format = format;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage = usage;
        info.samples = sample_count;
        info.sharingMode = share_mode;

        VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &image));
    }

    { // init memory
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(engine.device, image, &requirements);
        uint32_t memory_index = engine.get_memory_index(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.pNext = nullptr;
        info.memoryTypeIndex = memory_index;
        info.allocationSize = requirements.size;

        VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &memory));
    }

    VK_ASSERT(vkBindImageMemory(engine.device, image, memory, 0));

    { // init view
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.image = image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = format;
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.subresourceRange.aspectMask = aspect;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;

        VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &view));
    }
}

Texture::Texture(const Buffer& staging, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, 
    uint32_t mipmap, VkSampleCountFlagBits sample_count, VkSharingMode share_mode, VkImageAspectFlags aspect)
 : Texture(width, height, format, usage, mipmap, sample_count, share_mode, aspect) {
    
    VkCommandBuffer cmd_buffer;
    { // create new command buffer
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandPool = engine.graphics.pool;
        info.commandBufferCount = 1;

        vkAllocateCommandBuffers(engine.device, &info, &cmd_buffer);
    }

    { // record copy to cmd_buffer
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmd_buffer, &info);

        vkCmdCopyBufferToImage(cmd_buffer, staging.buffer, image, VK_IMAGE_LAYOUT_UNDEFINED, 0, nullptr);

        vkEndCommandBuffer(cmd_buffer);
    }
    
    {  // submit cmd_buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd_buffer;

        vkQueueSubmit(engine.graphics.queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(engine.graphics.queue);
    }

    vkFreeCommandBuffers(engine.device, engine.graphics.pool, 1, &cmd_buffer);
}

Texture::~Texture() { 
    if (image == nullptr) return;
    vkDestroyImageView(engine.device, view, nullptr);
    vkFreeMemory(engine.device, memory, nullptr);
    vkDestroyImage(engine.device, image, nullptr);
}
Texture::Texture(Texture&& other){ 
    image = other.image;
    memory = other.memory;
    view = other.view;
    other.image = nullptr;
}
Texture& Texture::operator=(Texture&& other){ 
    std::swap(image, other.image);
    std::swap(memory, other.memory);
    std::swap(view, other.view);
    return *this;
}

Model::Model(std::filesystem::path fp) {
    tinyobj::attrib_t obj_attrib;
    std::vector<tinyobj::shape_t> obj_shapes;
    std::vector<tinyobj::material_t> obj_materials;
    
    std::string warn, err;

    if (!tinyobj::LoadObj(&obj_attrib, &obj_shapes, &obj_materials, &warn, &err, fp.c_str())) {
        throw std::runtime_error(warn + err);
    }
    std::vector<Vertex>   vertex_data;
    std::vector<uint32_t> vertex_indices;
    std::vector<Material> material_data;
    std::vector<uint32_t> material_indices;
    std::vector<Texture> texture_data;

    std::unordered_map<Vertex, uint32_t> unique_vertices;
    
    for (auto& obj_shape : obj_shapes) {
        for (auto& vertex : obj_shape.mesh.indices) {
            Vertex vert;

            int position_index = 3 * vertex.vertex_index;
            vert.position = { 
                obj_attrib.vertices[position_index + 0],
                obj_attrib.vertices[position_index + 1],
                obj_attrib.vertices[position_index + 2]
            };

            int normal_index = 3 * vertex.normal_index;
            if (normal_index >= 0)
            {
                vert.normal = {
                    obj_attrib.normals[normal_index + 0],
                    obj_attrib.normals[normal_index + 1],
                    obj_attrib.normals[normal_index + 2]
                };
            }

            int texcoord_index = 2 * vertex.texcoord_index;
            if (texcoord_index >= 0) {
                vert.texcoord = {
                    obj_attrib.texcoords[texcoord_index + 0],
                    obj_attrib.texcoords[texcoord_index + 1]
                };
            }

            auto it = unique_vertices.find(vert);
            if (it == unique_vertices.end()) {
                it = unique_vertices.emplace_hint(it, vert, unique_vertices.size());
                vertex_data.push_back(vert);
            }

            vertex_indices.emplace_back(it->second);
        }
    }

    for (auto& obj_mat : obj_materials) {
        material_indices.push_back(material_data.size());
        auto& mat = material_data.emplace_back();
        if (obj_mat.metallic_texname == "") {
            mat.albedo = { obj_mat.diffuse[0], obj_mat.diffuse[1], obj_mat.diffuse[2] };
            mat.albedo_texture = -1;
        } else {
            // load texture
            mat.albedo_texture = texture_data.size();

            texture_data.emplace_back(
                obj_mat.metallic_texname,   // file path 
                VK_IMAGE_USAGE_SAMPLED_BIT, // usage 
                2,                          // mipmap
                VK_SAMPLE_COUNT_1_BIT,      // sample count
                VK_SHARING_MODE_EXCLUSIVE,  // share mode
                VK_IMAGE_ASPECT_COLOR_BIT   // image aspect
            );
        }
        if (obj_mat.metallic_texname == "") {
            mat.metallic = obj_mat.metallic;
            mat.metallic_texture = -1;
        } else {
            mat.albedo_texture = texture_data.size();

            texture_data.emplace_back(
                obj_mat.metallic_texname,   // file path
                VK_IMAGE_USAGE_SAMPLED_BIT, // usage 
                2,                          // mipmap
                VK_SAMPLE_COUNT_1_BIT,      // sample count
                VK_SHARING_MODE_EXCLUSIVE,  // share mode
                VK_IMAGE_ASPECT_COLOR_BIT   // image aspect
            );
        }
        if (obj_mat.roughness_texname == "") {
            mat.roughness = obj_mat.roughness;
               mat.roughness_texture = -1;
        } else {
            mat.roughness_texture = texture_data.size();

            texture_data.emplace_back(
                obj_mat.roughness_texname,  // file path
                VK_IMAGE_USAGE_SAMPLED_BIT, // usage 
                2,                          // mipmap
                VK_SAMPLE_COUNT_1_BIT,      // sample count
                VK_SHARING_MODE_EXCLUSIVE,  // share mode
                VK_IMAGE_ASPECT_COLOR_BIT   // image aspect
            );
        }
        if (obj_mat.normal_texname != "") {
            mat.normal_texture = texture_data.size();

            texture_data.emplace_back(
                obj_mat.roughness_texname,  // file path
                VK_IMAGE_USAGE_SAMPLED_BIT, // usage 
                2,                          // mipmap
                VK_SAMPLE_COUNT_1_BIT,      // sample count
                VK_SHARING_MODE_EXCLUSIVE,  // share mode
                VK_IMAGE_ASPECT_COLOR_BIT   // image aspect
            );
        } else {
            mat.normal_texture = -1;
        }
    }

    if (material_data.size() == 0) {
        material_indices.push_back(0);
        auto& mat = material_data.emplace_back();
        // vaguely white off colour
        mat.albedo = { 0.793, 0.793, 0.664 };
        mat.metallic = 0;
        mat.roughness = 1.5;
        mat.albedo_texture = -1;
        mat.metallic_texture = -1;
        mat.roughness_texture = -1;
        mat.normal_texture = -1;

        
    }

    // TODO: texture array...

    // buffers
    vertex = { vertex_data, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE };
    material = { material_data, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE };
    vertex_index = { vertex_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE };
    material_index = { material_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE };
}

// must specify the 2nd vertex buffer is per primitive not per vertex
// must bind the material buffer as a uniform/renderbuffer attachment
/* draw with material index 
VkBuffer vbos[] { vertex.buffer, material_index.buffer };
vkCmdBindVertexBuffers(cmd_buffer, 0, 2, vbos, 0);
vkCmdBindIndexBuffer(cmd_buffer, vertex_index.buffer, 0, VK_INDEX_TYPE_UINT32);
vkCmdDrawIndexed(cmd_buffer, vertex_index.size, material_index.size, 0, 0, 0);
*/