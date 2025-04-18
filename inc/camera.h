#pragma once
#include "vulkan.h"
#include "uniform.h"
#include <array>

struct Camera {
    struct Data {
        alignas(16) glm::mat4 proj;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 inv_proj;
        alignas(16) glm::vec3 eye;
        alignas(16) glm::uvec2 screen_size;
    };

    Camera();

    void bind(VK_TYPE(VkCommandBuffer) cmd_buffer, VK_TYPE(VkPipelineLayout) layout, uint32_t frame_index, VK_ENUM(VkPipelineBindPoint) bind_point);
    void update(uint32_t frame_index);

    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> buffer;
    std::array<UniformSet, MAX_FRAMES_IN_FLIGHT> uniform;

    glm::vec3 position = { 0.0f, 0.0f, 1.0f };
    glm::quat rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    float fov = 45.0f; // degrees
    float near = 0.1f;
    float far = 1000.0f;
};

extern Camera camera;