#pragma once
#include "vulkan.h"
#include "uniform.h"
#include <array>

struct Camera {
    struct Data {
        glm::mat4 projview;
        glm::vec3 position;
    private:
        float padding;
    };
    struct Buffer : UniformBuffer<Data>, UniformSet {
        Buffer();
    };

    void bind(VK_TYPE(VkCommandBuffer) cmd_buffer, VK_TYPE(VkPipelineLayout) layout, uint32_t frame_index, VK_ENUM(VkPipelineBindPoint) bind_point);
    void update(uint32_t frame_index);

    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> uniform;

    glm::vec3 position =  { 0.0f, 0.0f, 0.0f };
    glm::vec3 direction = { 0.0f, 0.0f, 1.0f }; // looking forward
    float fov = 0.785f; // 45 degrees
    float near = 0.1f;
    float far = 100.0f;
};

extern Camera camera;