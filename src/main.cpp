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

Settings    settings("cfg/settings.json");
Engine      engine;
Window      window;
Swapchain   swapchain;
Camera      camera;
Renderer    renderer;
Controller  controller;
std::vector<Model> models;
std::vector<Light> lights;

int main() {
    //{
    //    Model::Load("res/model/sponza/sponza.obj");
    //    for (auto& model : models) {
    //        model.transform.scale = glm::vec3(0.1f);
    //    }
    //}
    {
        Model::Load("res/model/cube/cube.obj");
        Model& floor = models.back();
        floor.transform.scale = glm::vec3(220.0f, 0.1f, 220.0f);
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
        
        //glm::ivec3 count{ 20, 10, 20 }; // 4000
        //glm::ivec3 count{ 10, 5, 10 }; // 500
        glm::ivec3 count{ 5, 2, 5 }; // 50
        for (int x = -count.x / 2; x < count.x / 2; ++x) 
            for (int y = 0; y < count.y; ++y) 
                for (int z = -count.z / 2; z < count.z / 2; ++z) {
                    uint32_t light_index = (x + count.x) + 
                                           (y + count.y) * count.x + 
                                           (z + count.z) * count.x * count.y;
                    lights.emplace_back(
                        glm::vec3(x + 1, y, z + 1) * 100.0f * glm::vec3(2, 1, 2) / glm::vec3(count), 400.0f / count.x, 
                        colours[light_index % colours.size()] * 5.0f, 2.0f
                    );
        }
    }
    
    while (!window.closed()) {
        window.update();
        renderer.draw();
    }

    VK_ASSERT(vkDeviceWaitIdle(engine.device));
}