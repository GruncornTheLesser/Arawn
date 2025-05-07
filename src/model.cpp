#define TINYOBJLOADER_IMPLEMENTATION
#define ARAWN_IMPLEMENTATION
#include "model.h"
#include "engine.h"
#include <cstring>
#include <algorithm>
#include <unordered_map>
#include <numeric>

namespace tinyobj {
    bool operator==(const tinyobj::index_t& lhs, const tinyobj::index_t& rhs) { 
        return lhs.vertex_index == rhs.vertex_index && 
            lhs.normal_index == rhs.normal_index && 
            lhs.texcoord_index == rhs.texcoord_index;
    }
}

namespace std {
    template<>
    struct hash<tinyobj::index_t> {
        size_t operator()(const tinyobj::index_t index) const { 
            return std::hash<glm::uvec3>()(glm::uvec3{ index.vertex_index, index.normal_index, index.texcoord_index });
        }
    };
}

void Model::Load(std::filesystem::path fp) {
    tinyobj::attrib_t obj_attrib;
    std::vector<tinyobj::shape_t> obj_models;
    std::vector<tinyobj::material_t> obj_materials;
    
    std::string warn, err;

    auto dir = fp.parent_path();

    if (!tinyobj::LoadObj(&obj_attrib, &obj_models, &obj_materials, &warn, &err, fp.string().c_str(),  dir.string().c_str(), true)) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& obj_model : obj_models) {
        std::vector<uint32_t> obj_indices;
        std::vector<uint32_t> obj_offsets;
        std::vector<Mesh> meshes;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<tinyobj::index_t, uint32_t> unique_vertices;
        
        // initialize // { 0, 1 ... n }
        obj_indices.resize(obj_model.mesh.num_face_vertices.size());
        std::ranges::iota(obj_indices, 0); 

        // filter out unsuported primitive types(lines and points)
        obj_indices.erase(std::remove_if(obj_indices.begin(), obj_indices.end(), 
            [&](uint32_t obj_face_index) {
                return obj_model.mesh.num_face_vertices[obj_face_index] < 3; 
            }
        ), obj_indices.end());
        
        // sorts by material_id
        std::ranges::sort(obj_indices, [&](uint32_t lhs, uint32_t rhs) {
            return obj_model.mesh.material_ids[lhs] > obj_model.mesh.material_ids[rhs];
        });

        // get vertex index offset for each face
        obj_offsets.resize(obj_model.mesh.num_face_vertices.size(), 0);
        for (uint32_t i = 1; i < obj_indices.size(); ++i) {
            obj_offsets[i] = obj_offsets[i - 1] + obj_model.mesh.num_face_vertices[obj_indices[i - 1]];
        }
        
        // generate triangle faces
        for (uint32_t obj_index : obj_indices) {
            uint32_t vertex_index = obj_offsets[obj_index]; // obj data vertex index
            uint32_t next_triangle = vertex_index + obj_model.mesh.num_face_vertices[obj_index];
            uint32_t next_face = vertex_index + obj_model.mesh.num_face_vertices[obj_index];
            
            for (; vertex_index < next_triangle; ++vertex_index) {
                auto& vertex_idx = obj_model.mesh.indices[vertex_index];
                auto res = unique_vertices.try_emplace(vertex_idx, vertices.size());
                indices.push_back(res.first->second);

                if (res.second) { // if new vertices
                    auto& vertex = vertices.emplace_back();
                    {
                        std::size_t position_index = vertex_idx.vertex_index * 3;
                        vertex.position = { obj_attrib.vertices[position_index + 0], 
                                            obj_attrib.vertices[position_index + 1], 
                                            obj_attrib.vertices[position_index + 2] 
                        };    
                    }

                    if (vertex_idx.normal_index != -1) {
                        std::size_t normal_index = vertex_idx.normal_index * 3;
                        vertex.normal = { obj_attrib.normals[normal_index + 0], 
                                          obj_attrib.normals[normal_index + 1], 
                                          obj_attrib.normals[normal_index + 2]
                        };
                    } else {
                        vertex.normal = { 0, 0, 0 };
                    }

                    if (vertex_idx.texcoord_index != -1) {
                        std::size_t texcoord_index = vertex_idx.texcoord_index * 2;
                        vertex.texcoord = { obj_attrib.texcoords[texcoord_index + 0], 
                                     1.0f - obj_attrib.texcoords[texcoord_index + 1]
                        };
                    } else {
                        vertex.texcoord = { 0, 0 };
                    }
                }
            }
        }
        
        // calculate face tangents/bitangents
        for (uint32_t i = 0; i < indices.size(); i+=3) {
            Vertex& v1 = vertices[indices[i + 0]]; 
            Vertex& v2 = vertices[indices[i + 1]];
            Vertex& v3 = vertices[indices[i + 2]];

            glm::vec3 edge1 = v2.position - v1.position;
            glm::vec3 edge2 = v3.position - v1.position;

            glm::vec2 delta1 = v2.texcoord - v1.texcoord;
            glm::vec2 delta2 = v3.texcoord - v1.texcoord;

            float f = 1.0f / (delta1.x * delta2.y - delta1.y * delta2.x);

            glm::vec3 tangent = f * (delta2.y * edge1 - delta1.y * edge2);
            glm::vec3 bi_tangent = f * (delta1.x * edge2 - delta2.x * edge1);

            tangent = glm::normalize(tangent);
            bi_tangent = glm::normalize(bi_tangent);

            v1.tangent += tangent;
            v2.tangent += tangent;
            v3.tangent += tangent;

            v1.bi_tangent += bi_tangent;
            v2.bi_tangent += bi_tangent;
            v3.bi_tangent += bi_tangent;
        }

        // normalize tangent/bi_tangent
        for (Vertex& vertex : vertices) {
            vertex.tangent = glm::normalize(vertex.tangent);
            vertex.bi_tangent = glm::normalize(vertex.bi_tangent);
        }

        { // get vertex groups for each mesh
            uint32_t vertex_count = 0;
            
            int curr_material = obj_model.mesh.material_ids[0];

            for (uint32_t obj_index : obj_indices) {
                vertex_count += obj_model.mesh.num_face_vertices[obj_index];
                int next_material = obj_model.mesh.material_ids[obj_index];
                
                if (next_material != curr_material) { // for each new material
                    meshes.emplace_back(vertex_count, Material{ &obj_materials[curr_material], dir });
                    
                    vertex_count = 0;
                    curr_material = next_material;
                }
            }
            
            // note only need to check if last default material becuse sorted by material_id
            if (curr_material == -1) {  // default material
                meshes.emplace_back(vertex_count);
            } else {
                meshes.emplace_back(vertex_count, Material{ &obj_materials[curr_material], dir });
            }
        }

        models.emplace_back(indices, vertices, std::move(meshes));
    }
}

Model::Model(const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices, std::vector<Mesh>&& meshes) 
 : vertex(vertices.data(), sizeof(Vertex) * vertices.size(), 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, std::array<uint32_t, 1>() = { engine.graphics.family }
    ),
    index(indices.data(), sizeof(uint32_t) * indices.size(), 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, std::array<uint32_t, 1>() = { engine.graphics.family }
    ),
    meshes(std::move(meshes)), 
    vertex_count(indices.size())
{ }

Model::Transform::Transform() {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        buffer[i] = Buffer(nullptr, sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, { });
        uniform[i] = UniformSet(engine.transform_layout, std::array<Uniform, 1>() = { UniformBuffer{ &buffer[i] } });
    }
}

void Model::Transform::update(uint32_t frame_index) {
    glm::mat4 transform = glm::identity<glm::mat4>();
    transform = glm::mat4_cast(rotation);
    transform = glm::scale(transform, scale);
    transform = glm::translate(transform, position);

    buffer[frame_index].set_value(&transform, sizeof(glm::mat4));
}