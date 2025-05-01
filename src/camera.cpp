#define ARAWN_IMPLEMENTATION
#include "camera.h"
#include "engine.h"
#include "uniform.h"
#include "swapchain.h"

Camera::Camera(float fov, float near, float far) : fov(fov), near(near), far(far) { 
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        buffer[i] = Buffer(nullptr, sizeof(Data),  
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
            std::array<uint32_t, 2>() = { engine.graphics.family, engine.compute.family }
        );

        uniform[i] = UniformSet(engine.camera_layout, std::array<Uniform, 1>() = { UniformBuffer{ &buffer[i] } });
        
        update(i);
    }

}

void Camera::update(uint32_t frame_index) {
    Data data;
    data.proj = glm::perspective(glm::radians(fov), (float)swapchain.extent.x / swapchain.extent.y, near, far);
    data.view = glm::mat4_cast(rotation);
    data.view = glm::translate(data.view, position);
    data.inv_proj = glm::inverse(data.proj);
    data.screen_size = swapchain.extent;
    data.near = near;
    data.far = far;
    data.eye = glm::inverse(data.view) * glm::vec4(0, 0, 0, 1);

    buffer[frame_index].set_value(&data, sizeof(Camera::Data));
}