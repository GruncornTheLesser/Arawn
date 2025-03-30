#define ARAWN_IMPLEMENTATION

#include "engine.h"
#include "window.h"
#include "swapchain.h"
#include "renderer.h"
#include "model.h"
#include "camera.h"

//#include "renderer/forward.h"
//#include "renderer/deferred.h"

/*
TODOLIST:
 * move paremeters to settings class
 * materials
 * lights
 * forward Swapchain
 * deferred Swapchain
 * add gui
 * settings menu
 * scene loading - https://casual-effects.com/data/
 * performance metrics
*/

float camera_speed = 5.0f;
void camera_move_forward(const Update& event) { 
    
    camera.position += glm::inverse(camera.rotation) * glm::vec3(0, 0, event.delta * camera_speed);
}

void camera_move_left(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(event.delta * camera_speed, 0, 0);
}

void camera_move_backward(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(0, 0, event.delta * -camera_speed);
}

void camera_move_right(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(event.delta * -camera_speed, 0, 0);
}

void camera_move_up(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(0.0, event.delta * camera_speed, 0);
}

void camera_move_down(const Update& event) { 
    camera.position += glm::inverse(camera.rotation) * glm::vec3(0.0, event.delta * -camera_speed, 0);
}

glm::vec2 rotate_camera_point;
glm::quat rotate_camera_rotation;
void camera_rotate(const Update& event) {
    glm::vec2 delta = window.mouse_position() - rotate_camera_point;
    camera.rotation = rotate_camera_rotation;
    
    glm::vec3 up = glm::vec3(0, 1, 0);
    glm::quat yaw = glm::angleAxis(-delta.x / swapchain.extent.x, up);
    camera.rotation *= yaw;
    
    glm::vec3 right = glm::inverse(camera.rotation) * glm::vec3(1, 0, 0);
    glm::quat pitch = glm::angleAxis(delta.y / swapchain.extent.y, right);
    
    camera.rotation *= pitch;
}


Settings    settings("configs/settings.json");
Engine      engine;
Window      window;
Swapchain   swapchain;
Renderer    renderer;
Camera      camera;
std::vector<Model> models = Model::Load("res/model/sponza/sponza.obj"); // "res/model/cube/cube.obj"

int main() {
    // toggle fullscreen hotkey
    window.on(KeyDown{ Key::F }) += [&](auto& event) { // toggle display mode
        window.set_display_mode(static_cast<DisplayMode>((static_cast<uint32_t>(window.get_display_mode()) + 1) % 3));
    };
    
    // switch resolution
    size_t index = 0;
    auto resolutions = window.enum_resolutions(4.0f / 3.0f);
    
    // resolution controls
    window.on(KeyDown{ Key::PLUS }) += [&](auto& event) { // increase resolution
        if (index == resolutions.size() - 1) return;
        window.set_resolution(resolutions[++index]);
    };

    window.on(KeyDown{ Key::MINUS }) += [&](auto& event) { // decrease resolution
        if(index == 0) return;
        window.set_resolution(resolutions[--index]);
    };
    
    // camera position controls
    
    uint32_t camera_move_forward_event_handle;
    uint32_t camera_move_left_event_handle;
    uint32_t camera_move_backward_event_handle;
    uint32_t camera_move_right_event_handle;
    uint32_t camera_move_up_event_handle;
    uint32_t camera_move_down_event_handle;
    
    window.on(KeyUp{ Key::W }) += [&](auto& event) { 
        window.on<Update>() -= camera_move_forward_event_handle;
    };
    window.on(KeyDown{ Key::W }) += [&](auto& event) { 
        camera_move_forward_event_handle = window.on<Update>() += camera_move_forward;
    };

    window.on(KeyUp{ Key::A }) += [&](auto& event) { 
        window.on<Update>() -= camera_move_left_event_handle;
    };
    window.on(KeyDown{ Key::A }) += [&](auto& event) { 
        camera_move_left_event_handle = window.on<Update>() += camera_move_left;
    };

    window.on(KeyUp{ Key::S }) += [&](auto& event) { 
        window.on<Update>() -= camera_move_backward_event_handle;
    };
    window.on(KeyDown{ Key::S }) += [&](auto& event) { 
        camera_move_backward_event_handle = window.on<Update>() += camera_move_backward;
    };

    window.on(KeyUp{ Key::D }) += [&](auto& event) { 
        window.on<Update>() -= camera_move_right_event_handle; 
    };
    window.on(KeyDown{ Key::D }) += [&](auto& event) { 
        camera_move_right_event_handle = window.on<Update>() += camera_move_right;
    };

    window.on(KeyUp{ Key::SPACE }) += [&](auto& event) { 
        window.on<Update>() -= camera_move_up_event_handle; 
    };
    window.on(KeyDown{ Key::SPACE }) += [&](auto& event) { 
        camera_move_up_event_handle = window.on<Update>() += camera_move_up;
    };

    window.on(KeyUp{ Key::L_SHIFT }) += [&](auto& event) { 
        window.on<Update>() -= camera_move_down_event_handle; 
    };
    window.on(KeyDown{ Key::L_SHIFT }) += [&](auto& event) { 
        camera_move_down_event_handle = window.on<Update>() += camera_move_down;
    };

    window.on(KeyDown{ Key::L_CTRL }) += [&](auto& event) { 
        camera_speed *= 5;
    };
    window.on(KeyUp{ Key::L_CTRL }) += [&](auto& event) { 
        camera_speed /= 5;
    };
    
    uint32_t camera_rotate_handle_event;
    window.on(MouseButtonUp{ Mouse::BUTTON_LEFT }) += [&](auto& event) { 
        window.on<Update>() -= camera_rotate_handle_event;
    };
    window.on(MouseButtonDown{ Mouse::BUTTON_LEFT }) += [&](auto& event) {
        rotate_camera_point = window.mouse_position();
        rotate_camera_rotation = camera.rotation;
        camera_rotate_handle_event = window.on<Update>() += camera_rotate;
    };

    std::ranges::for_each(models, [&](Model& model) { model.transform.scale = glm::vec3(0.1, 0.1, 0.1); });

    while (!window.closed()) { 
        window.update();
        renderer.draw();
    }

    vkDeviceWaitIdle(engine.device);
}