#pragma once
#include "vulkan.h"
#include "vertex.h"
#include "material.h"
#include <filesystem>

struct Light {
    glm::vec3 position; 
    float radius;
    glm::vec3 colour;
    float curve;
};

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
    Model(const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices, std::vector<Mesh>&& meshes);
    
    struct Transform {
        Transform();
        
        void update(uint32_t frame_index);

        glm::vec3 position { 0, 0, 0 };
        glm::quat rotation { 1, 0, 0, 0 };
        glm::vec3 scale = { 1, 1, 1 };
        
        std::array<UniformSet, MAX_FRAMES_IN_FLIGHT> uniform;
        std::array<Buffer, MAX_FRAMES_IN_FLIGHT> buffer;
    } transform;

private:
    uint32_t vertex_count = 0;
    Buffer vertex;
    Buffer index;
    std::vector<Mesh> meshes;
};

extern std::vector<Model> models;
extern std::vector<Light> lights;