#define ARAWN_IMPLEMENTATION

#include "engine.h"
#include "window.h"
#include "swapchain.h"
#include "renderer.h"
#include "model.h"
#include "camera.h"
#include "controller.h"

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

Settings    settings("configs/settings.json");
Engine      engine;
Window      window;
Swapchain   swapchain;
Renderer    renderer;
Camera      camera;
Controller  controller;
std::vector<Model> models;
std::vector<Light> mights;

Model* bunny;
float bunny_speed = 5.0f;
uint32_t bunny_up_handle;
uint32_t bunny_down_handle;
uint32_t bunny_left_handle;
uint32_t bunny_right_handle;
void bunny_move_up(const Update& ev) { bunny->transform.position.z += bunny_speed * ev.delta; }
void bunny_move_down(const Update& ev) { bunny->transform.position.z -= bunny_speed * ev.delta; }
void bunny_move_right(const Update& ev) { bunny->transform.position.x += bunny_speed * ev.delta; }
void bunny_move_left(const Update& ev) { bunny->transform.position.x -= bunny_speed * ev.delta; }



int main() {
    //Model::Load("res/model/sponza/sponza.obj");

    Model::Load("res/model/cube/cube.obj");
    {
        Model& floor = models.back();
        floor.transform.scale = glm::vec3(80.0f, 0.1f, 80.0f);
        floor.transform.position = glm::vec3(0, -5.0f, 0.0f);
    }

    while (!window.closed()) {
        window.update();
        renderer.draw();
    }

    vkDeviceWaitIdle(engine.device);
}