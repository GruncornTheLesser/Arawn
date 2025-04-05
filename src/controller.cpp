#include "controller.h"
#include "window.h"
#include "swapchain.h"
#include "camera.h"
#include "renderer.h"

Controller::Controller() {
    window.on(KeyUp{ Key::W })   += [&](auto& event) { window.on<Update>() -= move_forward_event_handle; };
    window.on(KeyDown{ Key::W }) += [&](auto& event) { move_forward_event_handle = window.on<Update>() += move_forward; };

    window.on(KeyUp{ Key::A })   += [&](auto& event) { window.on<Update>() -= move_left_event_handle; };
    window.on(KeyDown{ Key::A }) += [&](auto& event) { move_left_event_handle = window.on<Update>() += move_left; };

    window.on(KeyUp{ Key::S })   += [&](auto& event) { window.on<Update>() -= move_backward_event_handle; };
    window.on(KeyDown{ Key::S }) += [&](auto& event) { move_backward_event_handle = window.on<Update>() += move_backward; };

    window.on(KeyUp{ Key::D })   += [&](auto& event) { window.on<Update>() -= move_right_event_handle; };
    window.on(KeyDown{ Key::D }) += [&](auto& event) { move_right_event_handle = window.on<Update>() += move_right; };

    window.on(KeyUp{ Key::SPACE })   += [&](auto& event) { window.on<Update>() -= move_up_event_handle; };
    window.on(KeyDown{ Key::SPACE }) += [&](auto& event) { move_up_event_handle = window.on<Update>() += move_up; };

    window.on(KeyUp{ Key::L_SHIFT })   += [&](auto& event) { window.on<Update>() -= move_down_event_handle; };
    window.on(KeyDown{ Key::L_SHIFT }) += [&](auto& event) { move_down_event_handle = window.on<Update>() += move_down; };

    window.on(KeyUp{ Key::L_CTRL })   += [&](auto& event) { controller.move_speed /= 5; };
    window.on(KeyDown{ Key::L_CTRL }) += [&](auto& event) { controller.move_speed *= 5; };
    
    window.on(MouseButtonUp{ Mouse::BUTTON_LEFT }) += [&](auto& event) { window.on<Update>() -= rotate_event_handle; };
    window.on(MouseButtonDown{ Mouse::BUTTON_LEFT }) += [&](auto& event) {
        rotation_origin = window.mouse_position();
        rotation = camera.rotation;
        rotate_event_handle = window.on<Update>() += rotate;
    };

    window.on(KeyDown{ Key::F }) += [&](auto& event) {
        window.set_display_mode(static_cast<DisplayMode>((static_cast<uint32_t>(window.get_display_mode()) + 1) % 3));
    };
    
    // resolution controls
    window.on(KeyDown{ Key::PLUS }) += [&](auto& event) { // increase resolution
        auto resolutions = window.enum_resolutions(settings.aspect_ratio);
        resolution_index = (++resolution_index + resolutions.size()) % resolutions.size();
        window.set_resolution(resolutions[resolution_index]);
    };

    window.on(KeyDown{ Key::MINUS }) += [&](auto& event) { // decrease resolution
        auto resolutions = window.enum_resolutions(settings.aspect_ratio);
        resolution_index = (--resolution_index + resolutions.size()) % resolutions.size();
        window.set_resolution(resolutions[resolution_index]);
    };

    window.on(KeyDown{ Key::R }) += [&](auto& event) { // decrease resolution
        settings = Settings("configs/settings.json");
        renderer.recreate();
    };
}

void Controller::move_forward(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(0, 0, event.delta * controller.move_speed);
}

void Controller::move_left(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(event.delta * controller.move_speed, 0, 0);
}

void Controller::move_backward(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(0, 0, event.delta * -controller.move_speed);
}

void Controller::move_right(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(event.delta * -controller.move_speed, 0, 0);
}

void Controller::move_up(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(0.0, event.delta * controller.move_speed, 0);
}

void Controller::move_down(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(0.0, event.delta * -controller.move_speed, 0);
}

void Controller::rotate(const Update& event) {
    glm::vec2 delta = window.mouse_position() - controller.rotation_origin;
    camera.rotation = controller.rotation;
    
    glm::vec3 up = glm::vec3(0, 1, 0);
    glm::quat yaw = glm::angleAxis(-delta.x / swapchain.extent.x, up);
    camera.rotation *= yaw;
    
    glm::vec3 right = glm::inverse(camera.rotation) * glm::vec3(1, 0, 0);
    glm::quat pitch = glm::angleAxis(delta.y / swapchain.extent.y, right);
    
    camera.rotation *= pitch;
}