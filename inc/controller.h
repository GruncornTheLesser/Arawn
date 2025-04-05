#pragma once
#include "vulkan.h"
#include "events.h"

class Controller {
public:
    Controller();

private:
    static void move_forward(const Update& event);
    static void move_left(const Update& event);
    static void move_backward(const Update& event);
    static void move_right(const Update& event);
    static void move_up(const Update& event);
    static void move_down(const Update& event);
    static void rotate(const Update& event);

private:
    glm::vec2 rotation_origin;
    glm::quat rotation;
    float move_speed = 5.0f;
    uint32_t resolution_index;

    uint32_t move_forward_event_handle;
    uint32_t move_left_event_handle;
    uint32_t move_backward_event_handle;
    uint32_t move_right_event_handle;
    uint32_t move_up_event_handle;
    uint32_t move_down_event_handle;
    uint32_t rotate_event_handle;

};

extern Controller controller;