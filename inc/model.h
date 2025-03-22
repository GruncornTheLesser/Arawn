#pragma once
#include "vulkan.h"
#include "vertex.h"
#include "material.h"
#include <filesystem>

class Model {
public:
    struct Mesh { 
        uint32_t vertex_count;
        Material material;
    };

    static std::vector<Model> Load(std::filesystem::path fp);
    Model() { }
    Model(const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices, std::vector<Mesh>&& meshes);
    ~Model();
    Model(Model&& other);
    Model& operator=(Model&& other);
    Model(const Model& other) = delete;
    Model& operator=(const Model& other) = delete;
    

    void draw(VK_TYPE(VkCommandBuffer) cmd_buffer, VK_TYPE(VkPipelineLayout) layout, uint32_t frame_index);

private:
    VK_TYPE(VkBuffer) vertex_buffer = nullptr;
    VK_TYPE(VkDeviceMemory) vertex_memory;

    VK_TYPE(VkBuffer) index_buffer;
    VK_TYPE(VkDeviceMemory) index_memory;

    std::vector<Mesh> meshes;
};

extern Model model;