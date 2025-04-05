#define ARAWN_IMPLEMENTATION
#include "camera.h"
#include "engine.h"
#include "swapchain.h"

Camera::Uniform::Uniform()
 : buffer(nullptr, sizeof(Data)), set(engine.camera_layout, std::array<std::variant<UniformBuffer*, UniformTexture*>, 1>() = { &buffer }) 
{ }

void Camera::update(uint32_t frame_index) {
    Data data;
    data.proj = glm::perspective(glm::radians(fov), (float)swapchain.extent.x / swapchain.extent.y, near, far);
    data.view = glm::mat4_cast(rotation);
    data.view = glm::translate(data.view, position);
    data.eye = position;

    uniform[frame_index].buffer.set_value(&data);
}