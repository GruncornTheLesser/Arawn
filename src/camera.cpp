#define ARAWN_IMPLEMENTATION
#include "camera.h"
#include "engine.h"
#include "swapchain.h"

Camera::UBO::UBO() : UniformBuffer(nullptr), UniformSet(engine.camera_layout) { set_bindings( std::array<Uniform*, 1>{} = { this } ); }

void Camera::update(uint32_t frame_index) {
    Data data;
    data.proj = glm::perspective(glm::radians(fov), (float)swapchain.extent.x / swapchain.extent.y, near, far);
    data.view = glm::mat4_cast(rotation);
    data.view = glm::translate(data.view, position);

    data.test = frame_index;

    uniform[frame_index].set_value(data);
}