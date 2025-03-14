/*
#define VK_IMPLEMENTATION
#include "model.h"
#include "vertex.h"

#include <tiny_obj_loader.h>
#include <cstring>


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
            mat.albedo_texture = texture_data.size();
            texture_data.emplace_back(obj_mat.metallic_texname);
        }
        
        if (obj_mat.metallic_texname == "") {
            mat.metallic = obj_mat.metallic;
            mat.metallic_texture = -1;
        } else {
            mat.albedo_texture = texture_data.size();
            texture_data.emplace_back(obj_mat.metallic_texname);
        }

        if (obj_mat.roughness_texname == "") {
            mat.roughness = obj_mat.roughness;
            mat.roughness_texture = -1;
        } else {
            mat.roughness_texture = texture_data.size();
            texture_data.emplace_back(obj_mat.roughness_texname);
        }
        
        if (obj_mat.normal_texname != "") {
            mat.normal_texture = texture_data.size();
            texture_data.emplace_back(obj_mat.normal_texname);
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
    vertex = Buffer{ std::span{ vertex_data }, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    material = { std::span{ material_data }, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    vertex_index = { std::span{ vertex_indices }, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    material_index = { std::span{ material_indices }, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
}
*/

// must specify the 2nd vertex buffer is per primitive not per vertex
// must bind the material buffer as a uniform/renderbuffer attachment
/* draw with material index 
VkBuffer vbos[] { vertex.buffer, material_index.buffer };
vkCmdBindVertexBuffers(cmd_buffer, 0, 2, vbos, 0);
vkCmdBindIndexBuffer(cmd_buffer, vertex_index.buffer, 0, VK_INDEX_TYPE_UINT32);
vkCmdDrawIndexed(cmd_buffer, vertex_index.size, material_index.size, 0, 0, 0);
*/