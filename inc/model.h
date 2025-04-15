#pragma once
#include "vulkan.h"
#include "vertex.h"
#include "material.h"
#include <filesystem>

class Model {
public:
    friend class Renderer;
    friend class DepthPass;
    friend class CullingPass;
    friend class DeferredPass;
    friend class ForwardPass;

    struct Mesh { 
        uint32_t vertex_count;
        Material material;
    };

    static void Load(std::filesystem::path fp);
    Model() : vertex_buffer(nullptr) { }
    Model(const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices, std::vector<Mesh>&& meshes);
    ~Model();
    Model(Model&& other);
    Model& operator=(Model&& other);
    Model(const Model& other) = delete;
    Model& operator=(const Model& other) = delete;
    
    struct Transform {
        struct Uniform { 
            UniformBuffer buffer;
            UniformSet set; 
            Uniform();
        };

        void update(uint32_t frame_index);

        glm::vec3 position { 0, 0, 0 };
        glm::quat rotation { 1, 0, 0, 0 };
        glm::vec3 scale = { 1, 1, 1 };
        
        std::array<Uniform, MAX_FRAMES_IN_FLIGHT> uniform;
    } transform;
private:
    uint32_t vertex_count = 0;
    VK_TYPE(VkBuffer) vertex_buffer = nullptr;
    VK_TYPE(VkDeviceMemory) vertex_memory;

    VK_TYPE(VkBuffer) index_buffer;
    VK_TYPE(VkDeviceMemory) index_memory;

    std::vector<Mesh> meshes;
};

extern std::vector<Model> models;