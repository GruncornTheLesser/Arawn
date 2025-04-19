#pragma once
#include "vulkan.h"
#include "uniform.h"
#include <array>

struct Camera {
    struct Data {
        alignas(16) glm::mat4 proj;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 inv_proj;
        alignas(16) glm::uvec2 screen_size;
        alignas(4) float near;
        alignas(4) float far;
        alignas(16) glm::vec3 eye;
    };

    Camera();

    void update(uint32_t frame_index);

    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> buffer;
    std::array<UniformSet, MAX_FRAMES_IN_FLIGHT> uniform;

    glm::vec3 position = { 0.0f, 0.0f, 1.0f };
    glm::quat rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    float fov = 70.0f; // degrees
    float near = 1.0f;
    float far = 1000.0f;
};

extern Camera camera;