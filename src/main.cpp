#define ARAWN_IMPLEMENTATION

#include "engine.h"
#include "window.h"
#include "swapchain.h"
#include "renderer.h"
#include "model.h"
#include "camera.h"
#include "controller.h"

//#include "pass/depth.h"
//#include "pass/deferred.h"
//#include "pass/culling.h"
//#include "pass/forward.h"


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
std::vector<Light> lights;

int main() {
    //Model::Load("res/model/sponza/sponza.obj");

    {
        Model::Load("res/model/cube/cube.obj");
        Model& floor = models.back();
        floor.transform.scale = glm::vec3(8.0f, 0.1f, 8.0f);
        floor.transform.position = glm::vec3(0, -0.05f, 0.0f);
    }
    
    {
        std::array<glm::vec3, 7> colours = { 
            glm::vec3(0, 0, 1),
            glm::vec3(0, 1, 0),
            glm::vec3(0, 1, 1),
            glm::vec3(1, 0, 0),
            glm::vec3(1, 1, 1),
            glm::vec3(1, 1, 0),
            glm::vec3(1, 1, 1)  
        };

        for (int x = -5; x < 5; ++x) {
            for (int y = -5; y < 5; ++y) {
                lights.emplace_back(glm::vec3(x, 0.1f, y), 1.5f, 2.0f * colours[((y + 5) * 9 + x + 5) % 7], 2.0f);
            }
        }
        
    }
    
    while (!window.closed()) {
        window.update();
        renderer.draw();
    }

    vkDeviceWaitIdle(engine.device);
}