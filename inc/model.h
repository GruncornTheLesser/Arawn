#pragma once
#include "vulkan.h"
#include <vector>
#include <filesystem>

struct Vertex { 
    glm::vec3 position; 
    glm::vec2 texcoord;
    glm::vec3 normal; 
    glm::vec4 tangent;

    bool operator==(const Vertex& other) const {
        return position == other.position && 
               texcoord == other.texcoord && 
               normal == other.normal && 
               tangent == other.tangent;
    }
};

struct Material { 
    glm::vec3 albedo;
    float roughness;
    float metallic;
    int albedo_texture;
    int normal_texture;
    int roughness_texture;
    int metallic_texture;
};

namespace std { // hash for vertex
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return hash<glm::vec3>()(vertex.position) ^ 
                hash<glm::vec2>()(vertex.texcoord) ^ 
                hash<glm::vec2>()(vertex.normal) ^ 
                hash<glm::vec2>()(vertex.tangent);
        }
    };
}

class Buffer {
public:
    template<typename T> 
    Buffer(const std::vector<T>& vec, VK_ENUM(VkBufferUsageFlags) usage, VK_ENUM(VkMemoryPropertyFlags) properties, VK_ENUM(VkSharingMode) sharing_mode)
     : Buffer(vec.data(), sizeof(T) * vec.size(), usage, properties, sharing_mode) { }
    Buffer() : buffer(nullptr) { }
    Buffer(size_t buffer_size, VK_ENUM(VkBufferUsageFlags) usage, VK_ENUM(VkMemoryPropertyFlags) properties, VK_ENUM(VkSharingMode) sharing_mode);
    Buffer(const void* data, size_t buffer_size, VK_ENUM(VkBufferUsageFlags) usage, VK_ENUM(VkMemoryPropertyFlags) properties, VK_ENUM(VkSharingMode) sharing_mode);
    ~Buffer();

    Buffer(Buffer&& other);
    Buffer& operator=(Buffer&& other);

    VK_TYPE(VkBuffer) buffer;
    VK_TYPE(VkDeviceMemory) memory;
    size_t size;
};

class Texture {
public:
    Texture() : image(nullptr) { }
    Texture(std::filesystem::path fp, VK_ENUM(VkImageUsageFlags) usage, uint32_t mipmap, 
        VK_ENUM(VkSampleCountFlagBits) sample_count, VK_ENUM(VkSharingMode) share_mode, 
        VK_ENUM(VkImageAspectFlags) aspect);
    Texture(uint32_t width, uint32_t height, VK_ENUM(VkFormat) format, VK_ENUM(VkImageUsageFlags) usage, 
        uint32_t mipmap, VK_ENUM(VkSampleCountFlagBits) sample_count, VK_ENUM(VkSharingMode) share_mode, 
        VK_ENUM(VkImageAspectFlags) aspect);
    Texture(const Buffer& staging, uint32_t width, uint32_t height, VK_ENUM(VkFormat) format, 
        VK_ENUM(VkImageUsageFlags) usage, uint32_t mipmap, VK_ENUM(VkSampleCountFlagBits) sample_count, 
        VK_ENUM(VkSharingMode) share_mode, VK_ENUM(VkImageAspectFlags) aspect);
    
    ~Texture();
    Texture(Texture&&);
    Texture& operator=(Texture&&);
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    VK_TYPE(VkImage) image;
    VK_TYPE(VkDeviceMemory) memory;
    VK_TYPE(VkImageView) view;
};

class Model {
public:
    Model() { }
    Model(std::filesystem::path fp);

private:
    // texture buffer
    Buffer vertex, material, vertex_index, material_index;
    std::vector<Texture> textures;

};