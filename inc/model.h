#pragma once
#include "vulkan.h"
#include <unordered_map>
#include <filesystem>

struct Material { 

};

class Model {
public:
    Model() { }
    Model(std::filesystem::path fp);

private:
    struct Buffer { 
        VK_TYPE(VkBuffer) buffer;
        VK_TYPE(VkDeviceMemory) memory;
    };

    struct UniformBuffer : Buffer {
        VK_TYPE(VkDescriptorSet) set;
    };

    struct Transform : UniformBuffer {
        struct UBO {
            glm::mat4 transform;
            glm::vec3 position;
            glm::quat rotation;
            glm::vec3 scale;
        };
        
    } transform;

    struct Texture {
        VK_TYPE(VkImage) image;
        VK_TYPE(VkDeviceMemory) memory;
        VK_TYPE(VkImageView) view;
    };

    struct Material : UniformBuffer {
        struct UBO {
            glm::vec3 albedo;
            float roughness;
            float metallic;

            std::shared_ptr<Texture> albedo_texture;
            std::shared_ptr<Texture> normal_texture;
            std::shared_ptr<Texture> roughness_texture;
            std::shared_ptr<Texture> metallic_texture;
        };
    };


    // texture buffer
    Buffer vertices;
    UniformBuffer indices;
    UniformBuffer materials;
};

extern Model model;