#pragma once
#include "vulkan.h"
#include "swapchain.h"
#include <vector>

// TODO: work out padding/packing

struct Camera {
    glm::mat4 projview;
    glm::vec3 position;
    // padding ???
};

struct Light {
    glm::vec3 position;
    float radius;
    glm::vec3 intensity;
private:
    float padding;
};

class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(Renderer&&);
    Renderer& operator=(Renderer&&);
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void draw();

    void recreate();

    void record();

private:
    struct Attachment {
        Attachment();
        ~Attachment();
        Attachment(Attachment&& other);
        Attachment& operator=(Attachment&& other);
        Attachment(const Attachment& other) = delete;
        Attachment& operator=(const Attachment& other) = delete;
        
        VK_TYPE(VkImage) image;
        VK_TYPE(VkImageView) view;
        VK_TYPE(VkDeviceMemory) memory;
    };

    VK_TYPE(VkFramebuffer)* framebuffers; // array of framebuffers, 
    
    VK_TYPE(VkDescriptorSetLayout) object_set_layout;
    VK_TYPE(VkDescriptorSetLayout) camera_set_layout;
    VK_TYPE(VkDescriptorSetLayout) material_set_layout;
    VK_TYPE(VkDescriptorSetLayout) light_set_layout; // lights and clusters
    
    Attachment resolve_attachment;  // enabled when anti alias not NONE
    Attachment depth_attachment;    // enabled when depth mode not NONE
    Attachment position_attachment, normal_attachment, albedo_attachment; // enabled when render mode DEFERRED

    struct {
        VK_TYPE(VkCommandBuffer) cmd_buffer;
        VK_TYPE(VkRenderPass) renderpass;
        VK_TYPE(VkPipelineLayout) pipeline_layout; // obj, cam
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkSemaphore) finished;
    } depth_pass;

    struct { // compute pass, generates clusters/tiles
        VK_TYPE(VkCommandBuffer) cmd_buffer;
        VK_TYPE(VkPipelineLayout) pipeline_layout; // viewport, clusters
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkSemaphore) finished;
    } cluster_pass;

    struct { // compute pass, cull lights outside of cluster/tiles
        VK_TYPE(VkCommandBuffer) cmd_buffer;
        VK_TYPE(VkPipelineLayout) pipeline_layout; // cam, lights, clusters
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkSemaphore) finished;
    } culling_pass;

    struct {
        VK_TYPE(VkCommandBuffer) cmd_buffer;
        VK_TYPE(VkRenderPass) renderpass;

        VK_TYPE(VkPipelineLayout) pipeline_layout;
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkSemaphore) finished;
    } deferred_pass;

    struct {
        VK_TYPE(VkCommandBuffer) cmd_buffer;
        VK_TYPE(VkRenderPass) renderpass;
        VK_TYPE(VkPipelineLayout) pipeline_layout;
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkSemaphore) finished;
    } forward_pass;
};

/*
    
*/