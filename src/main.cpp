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

std::vector<Model> models = Model::Load("res/model/cube/cube.obj");

int main() {
    std::ranges::for_each(models, [&](Model& model) { 
        model.transform.scale = glm::vec3(0.1, 0.1, 0.1);
    });

    while (!window.closed()) {
        window.update();
        renderer.draw();
    }

    vkDeviceWaitIdle(engine.device);
}