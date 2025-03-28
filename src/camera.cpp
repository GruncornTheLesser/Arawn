#define ARAWN_IMPLEMENTATION
#include "camera.h"
#include "engine.h"
#include "swapchain.h"

Camera::Buffer::Buffer()
 : UniformBuffer<Data>(nullptr), UniformSet(engine.camera_layout, std::array<Uniform*, 1>()= { this }) { }

void Camera::update(uint32_t frame_index) {
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)swapchain.extent.x / swapchain.extent.y, 0.01f, 100.0f);
    glm::mat4 view = glm::lookAt({ 2, 0.5f, 2 }, { 0, 0, 0 }, glm::vec3(0, 1, 0));
    
    Data data;
    data.projview = proj * view; //glm::transpose(glm::inverse());
    data.position = position;
    uniform[frame_index].set(&data);
}

/*
-1.8, 0.0, 0.0, 0.0
 0.0, 2.4, 0.0, 0.0
 0.0, 0.0, 1.0, 1.0
 0.0, 0.0, 0.1, 0.0
*/