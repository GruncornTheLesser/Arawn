#define ARAWN_IMPLEMENTATION

#include "engine.h"
#include "window.h"
#include "swapchain.h"
#include "renderer.h"
#include "model.h"
#include "camera.h"
#include "controller.h"
#include <glm/gtx/component_wise.hpp>

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
    {
        Model::Load("res/model/sponza/sponza.obj");
        for (auto& model : models) {
            model.transform.scale = glm::vec3(0.1f);
        }
    }
    //{
    //    Model::Load("res/model/cube/cube.obj");
    //    Model& floor = models.back();
    //    floor.transform.scale = glm::vec3(220.0f, 0.1f, 220.0f);
    //    floor.transform.position = glm::vec3(0, -0.05f, 0.0f);      
    //}
    
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
        
        glm::ivec3 count{ 20, 10, 20 }; // 4000
        //glm::ivec3 count{ 15, 6, 15 }; // 1350
        //glm::ivec3 count{ 15, 6, 15 }; // 1350
        //glm::ivec3 count{ 10, 5, 10 }; // 500
        //glm::ivec3 count{ 3, 2, 3 }; // 50
        for (int x = 0; x < count.x; ++x) for (int y = 0; y < count.y; ++y) for (int z = 0; z < count.z; ++z) {
            lights.emplace_back(
                glm::vec3(x - (count.x - 1) / 2.0f, y, z - (count.z - 1) / 2.0f) / glm::vec3(count) * 200.0f, 1.5f * 200.0f / glm::compMax(count), 
                colours[rand() % colours.size()] * 5.0f, 2.0f
            );
        }
    }
    
    while (!window.closed()) {
        window.update();
        renderer.draw();
    }

    VK_ASSERT(vkDeviceWaitIdle(engine.device));
}