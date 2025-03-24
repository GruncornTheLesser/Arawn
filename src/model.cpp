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
        return lhs.vertex_index ==   rhs.vertex_index && 
            lhs.normal_index ==   rhs.normal_index && 
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

std::vector<Model> Model::Load(std::filesystem::path fp) {
    tinyobj::attrib_t obj_attrib;
    std::vector<tinyobj::shape_t> obj_models;
    std::vector<tinyobj::material_t> obj_materials;
    
    std::string warn, err;

    if (!tinyobj::LoadObj(&obj_attrib, &obj_models, &obj_materials, &warn, &err, fp.c_str())) {
        throw std::runtime_error(warn + err);
    }
    
    std::vector<Model> models;

    for (const auto& obj_model : obj_models) {
        std::vector<uint32_t> face_indices;
        std::vector<uint32_t> face_offsets;
        std::vector<Mesh> meshes;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<tinyobj::index_t, uint32_t> unique_vertices;

        // initialize // { 0, 1 ... n }
        face_indices.resize(obj_model.mesh.num_face_vertices.size());
        std::ranges::iota(face_indices, 0); 

        // filter out unsuported primitive types(lines and points)
        face_indices.erase(std::remove_if(face_indices.begin(), face_indices.end(), 
            [&](uint32_t obj_face_index) {
                return obj_model.mesh.num_face_vertices[obj_face_index] < 3; 
            }
        ), face_indices.end());
        
        // sorts by material id
        std::ranges::sort(face_indices, [&](uint32_t lhs, uint32_t rhs) {
            return obj_model.mesh.material_ids[lhs] > obj_model.mesh.material_ids[rhs];
        });

        // get vertex index offset for each face
        face_offsets.resize(obj_model.mesh.num_face_vertices.size(), 0);
        for (uint32_t i = 1; i < face_indices.size(); ++i) {
            face_offsets[i] = face_offsets[i - 1] + obj_model.mesh.num_face_vertices[face_indices[i - 1]];
        }
        
        for (uint32_t face_index : face_indices) {
            uint32_t vertex_index = face_offsets[face_index]; // obj data vertex index
            uint32_t next_triangle = vertex_index + obj_model.mesh.num_face_vertices[face_index];
            uint32_t next_face = vertex_index + obj_model.mesh.num_face_vertices[face_index];
            
            for (; vertex_index < next_triangle; ++vertex_index) { // triangle
                auto& vertex_idx = obj_model.mesh.indices[vertex_index];
                auto res = unique_vertices.try_emplace(vertex_idx, vertices.size());
                indices.push_back(res.first->second);

                if (res.second) { // if new vertices
                    auto& vertex = vertices.emplace_back();
                    if (vertex_idx.vertex_index != -1) {
                        std::size_t position_index = vertex_idx.vertex_index * 3;
                        vertex.position = { obj_attrib.texcoords[position_index + 0], 
                                            obj_attrib.texcoords[position_index + 1], 
                                            obj_attrib.texcoords[position_index + 2] 
                        };
                    }

                    if (vertex_idx.normal_index != -1) {
                        std::size_t normal_index = vertex_idx.normal_index * 2;
                        vertex.normal = { obj_attrib.texcoords[normal_index + 0], 
                                          obj_attrib.texcoords[normal_index + 1], 
                                          obj_attrib.texcoords[normal_index + 2]
                        };
                    }

                    if (vertex_idx.texcoord_index != -1) {
                        std::size_t texcoord_index = vertex_idx.texcoord_index * 2;
                        vertex.texcoord = { obj_attrib.texcoords[texcoord_index + 0], 
                                     1.0f - obj_attrib.texcoords[texcoord_index + 1]
                        };
                    }
                }
            }
                
            for (; vertex_index < next_face; ++vertex_index) { // polygon
                // redraw final line of last triangle
                indices.push_back(*(indices.end() - 3)); // { a, b, c }, { a, c, d }
                indices.push_back(*(indices.end() - 2));
                
                auto& vertex_idx = obj_model.mesh.indices[vertex_index];
                auto res = unique_vertices.try_emplace(vertex_idx, vertices.size());
                indices.push_back(res.first->second);

                if (res.second) { // if new vertices
                    auto& vertex = vertices.emplace_back();
                    if (vertex_idx.vertex_index != -1) {
                        std::size_t position_index = vertex_idx.vertex_index * 3;
                        vertex.position = { obj_attrib.texcoords[position_index + 0], 
                                            obj_attrib.texcoords[position_index + 1], 
                                            obj_attrib.texcoords[position_index + 2] 
                        };
                    }

                    if (vertex_idx.normal_index != -1) {
                        std::size_t normal_index = vertex_idx.normal_index * 2;
                        vertex.normal = { obj_attrib.texcoords[normal_index + 0], 
                                          obj_attrib.texcoords[normal_index + 1], 
                                          obj_attrib.texcoords[normal_index + 2]
                        };
                    }

                    if (vertex_idx.texcoord_index != -1) {
                        std::size_t texcoord_index = vertex_idx.texcoord_index * 2;
                        vertex.texcoord = { obj_attrib.texcoords[texcoord_index + 0], 
                                     1.0f - obj_attrib.texcoords[texcoord_index + 1]
                        };
                    }
                }
            }
        }
        
        { // get vertex groups for each mesh
            uint32_t vertex_count = 0;
            int curr_material = obj_model.mesh.material_ids[0];

            for (uint32_t face_index : face_indices) {
                vertex_count += obj_model.mesh.num_face_vertices[face_index];
                int next_material = obj_model.mesh.material_ids[face_index];
                
                if (next_material != curr_material) { // for each new material
                    meshes.emplace_back(vertex_count, Material{ &obj_materials[curr_material] });
                    
                    vertex_count = 0;
                    curr_material = next_material;
                }
            }
            
            if (curr_material == -1) {  // default material
                meshes.emplace_back(vertex_count);
                // note only need to check if last default material becuse sorted by material_id
            } else {
                meshes.emplace_back(vertex_count, Material{ &obj_materials[curr_material] });
            }

        }

        models.emplace_back(indices, vertices, std::move(meshes));
    }
    return models;    
}

Model::Model(const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices, std::vector<Mesh>&& meshes) 
 : meshes(std::move(meshes))
{
    VkDeviceSize vertex_buffer_size = sizeof(Vertex) * vertices.size();
    VkDeviceSize index_buffer_size = sizeof(uint32_t) * indices.size();

    // temporaries to copy data to buffers
    VkCommandBuffer cmd_buffer;
    VkFence cmd_finished;
    VkBuffer vertex_staging_buffer;
    VkDeviceMemory vertex_staging_memory;
    VkBuffer index_staging_buffer;
    VkDeviceMemory index_staging_memory;

    { // allocate temporary cmd buffer
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandPool = engine.graphics.pool;
        info.commandBufferCount = 1;

        VK_ASSERT(vkAllocateCommandBuffers(engine.device, &info, &cmd_buffer));
    }

    { // create temporary fence
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;

        VK_ASSERT(vkCreateFence(engine.device, &info, nullptr, &cmd_finished));
    }



    { // create temporary vertex staging buffer
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.flags = 0;
        info.pNext = nullptr;
        info.pQueueFamilyIndices = &engine.graphics.family;
        info.queueFamilyIndexCount = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.size = vertex_buffer_size;
        info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VK_ASSERT(vkCreateBuffer(engine.device, &info, nullptr, &vertex_staging_buffer));
    }

    { // allocate temporary vertex staging memory
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(engine.device, vertex_staging_buffer, &requirements);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &vertex_staging_memory));
        
        VK_ASSERT(vkBindBufferMemory(engine.device, vertex_staging_buffer, vertex_staging_memory, 0));
    }

    { // write to vertex buffer
        void* buffer_data;
        VK_ASSERT(vkMapMemory(engine.device, vertex_staging_memory, 0, vertex_buffer_size, 0 , &buffer_data));
        memcpy(buffer_data, vertices.data(), vertex_buffer_size);
        vkUnmapMemory(engine.device, vertex_staging_memory);
    }

    { // create vertex buffer
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.flags = 0;
        info.pNext = nullptr;
        info.pQueueFamilyIndices = &engine.graphics.family;
        info.queueFamilyIndexCount = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.size = vertex_buffer_size;
        info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VK_ASSERT(vkCreateBuffer(engine.device, &info, nullptr, &vertex_buffer));
    }

    { // allocate vertex memory
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(engine.device, vertex_buffer, &requirements);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &vertex_memory));

        VK_ASSERT(vkBindBufferMemory(engine.device, vertex_buffer, vertex_memory, 0));
    }



    { // create temporary index staging buffer
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.flags = 0;
        info.pNext = nullptr;
        info.pQueueFamilyIndices = &engine.graphics.family;
        info.queueFamilyIndexCount = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.size = index_buffer_size;
        info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VK_ASSERT(vkCreateBuffer(engine.device, &info, nullptr, &index_staging_buffer));
    }

    { // allocate temporary index staging memory
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(engine.device, index_staging_buffer, &requirements);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &index_staging_memory));   
        
        VK_ASSERT(vkBindBufferMemory(engine.device, index_staging_buffer, index_staging_memory, 0));
    }

    { // write to index buffer
        void* buffer_data;
        VK_ASSERT(vkMapMemory(engine.device, index_staging_memory, 0, index_buffer_size, 0 , &buffer_data));
        memcpy(buffer_data, vertices.data(), index_buffer_size);
        vkUnmapMemory(engine.device, index_staging_memory);
    }

    { // create index buffer
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.flags = 0;
        info.pNext = nullptr;
        info.pQueueFamilyIndices = &engine.graphics.family;
        info.queueFamilyIndexCount = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.size = index_buffer_size;
        info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VK_ASSERT(vkCreateBuffer(engine.device, &info, nullptr, &index_buffer));
    }

    { // allocate index memory
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(engine.device, index_buffer, &requirements);

        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &index_memory));   
    
        VK_ASSERT(vkBindBufferMemory(engine.device, index_buffer, index_memory, 0));
    }



    { // begin cmd buffer
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer, &info));
    }
    
    { // copy staging to vertex buffer
        VkBufferCopy region{};
        region.size = vertex_buffer_size;
        vkCmdCopyBuffer(cmd_buffer, vertex_staging_buffer, vertex_buffer, 1, &region);
    }

    { // copy staging to index buffer
        VkBufferCopy region{};
        region.size = index_buffer_size;
        vkCmdCopyBuffer(cmd_buffer, index_staging_buffer, index_buffer, 1, &region);
    }

    { // end cmd buffer
        VK_ASSERT(vkEndCommandBuffer(cmd_buffer));

        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &cmd_buffer;

        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, cmd_finished));
    }
    
    VK_ASSERT(vkWaitForFences(engine.device, 1, &cmd_finished, VK_TRUE, UINT64_MAX));

    vkDestroyBuffer(engine.device, vertex_staging_buffer, nullptr);
    vkFreeMemory(engine.device, vertex_staging_memory, nullptr);
    vkDestroyBuffer(engine.device, index_staging_buffer, nullptr);
    vkFreeMemory(engine.device, index_staging_memory, nullptr);
    vkFreeCommandBuffers(engine.device, engine.graphics.pool, 1, &cmd_buffer);
    vkDestroyFence(engine.device, cmd_finished, nullptr);
}

Model::Model(Model&& other) {
    vertex_buffer = other.vertex_buffer;
    vertex_memory = other.vertex_memory;
    index_buffer = other.index_buffer;
    index_memory = other.index_memory;
    
    //transform = std::move(other.transform);
    meshes = std::move(other.meshes);
    
    other.vertex_buffer = nullptr;
}

Model::~Model() {
    if (vertex_buffer == nullptr) return;

    vkDestroyBuffer(engine.device, vertex_buffer, nullptr);
    vkFreeMemory(engine.device, vertex_memory, nullptr);
    vkDestroyBuffer(engine.device, index_buffer, nullptr);
    vkFreeMemory(engine.device, index_memory, nullptr);
}

Model& Model::operator=(Model&& other) {
    std::swap(vertex_buffer, vertex_buffer);
    std::swap(vertex_memory, vertex_memory);
    std::swap(index_buffer, index_buffer);
    std::swap(index_memory, index_memory);

    //std::swap(transform, other.transform);
    std::swap(meshes, other.meshes);

    return *this;
}
