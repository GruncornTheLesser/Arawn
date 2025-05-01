#define ARAWN_IMPLEMENTATION

#include "engine.h"
#include "window.h"
#include "swapchain.h"
#include "renderer.h"
#include "model.h"
#include "camera.h"
#include "controller.h"
#include <glm/gtx/component_wise.hpp>
#include <iostream>
#include <fstream>

Settings    settings("cfg/settings.json");
Engine      engine;
Window      window;
Swapchain   swapchain;
Camera      camera(70.0f, 1.0f, 300.0f);
Renderer    renderer;
Controller  controller;
std::vector<Model> models;
std::vector<Light> lights;

void log(std::filesystem::path fp, std::string header, const std::vector<std::chrono::nanoseconds>& data) {
    std::cout << "finished " << header << std::endl; 
    
    std::ofstream ofs(fp, std::ios_base::out | std::ios_base::app);
    ofs << header << ", ";
    for (auto& i : data) {
        ofs << i << ", ";
    }
    ofs << std::endl;
    ofs.close();
}

void test(std::vector<std::chrono::nanoseconds>& samples) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (uint32_t i = 0; i < samples.size(); ++i) {
        auto next = std::chrono::high_resolution_clock::now();
        
        window.update();
        renderer.draw();

        samples[i] = next - start;
        start = next;
    }
    VK_ASSERT(vkDeviceWaitIdle(engine.device));
}

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
        for (int x = 0; x < count.x; ++x) for (int y = 0; y < count.y; ++y) for (int z = 0; z < count.z; ++z) {
            lights.emplace_back(
                glm::vec3(x - (count.x - 1) / 2.0f, y, z - (count.z - 1) / 2.0f) / glm::vec3(count) * 200.0f, 1.5f * 200.0f / glm::compMax(count), 
                colours[rand() % colours.size()] * 5.0f, 2.0f
            );
        }
    }

    //while (!window.closed()) {
    //    window.update();
    //    renderer.draw();
    //}
    //VK_ASSERT(vkDeviceWaitIdle(engine.device));
    //return 0;

    std::vector<std::chrono::nanoseconds> samples(5000);
    std::string row_name = "";
    settings.set_vsync_mode(VsyncMode::DISABLED);

    for (RenderMode render_mode : std::array<RenderMode, 2>{ RenderMode::FORWARD, RenderMode::DEFERRED }) {
        settings.set_render_mode(render_mode);

        for (CullingMode culling_mode : std::array<CullingMode, 3>{ CullingMode::DISABLED, CullingMode::CLUSTERED, CullingMode::TILED }) {
            settings.set_culling_mode(culling_mode);
            
            for (DepthMode depth_mode : std::array<DepthMode, 2>{ DepthMode::DISABLED, DepthMode::ENABLED }) {
                settings.set_depth_mode(depth_mode);

                for (AntiAlias antialias : std::array<AntiAlias, 4>{ AntiAlias::NONE, AntiAlias::MSAA_2, AntiAlias::MSAA_4, AntiAlias::MSAA_8 }) {
                    settings.set_anti_alias_mode(antialias);
                    
                    switch (render_mode) {
                        case RenderMode::FORWARD: row_name = "forward"; break;
                        case RenderMode::DEFERRED: row_name = "deferred"; break;
                    }
                    switch (culling_mode) {
                        case CullingMode::CLUSTERED: row_name = "clustered " + row_name + " plus"; break;
                        case CullingMode::TILED: row_name = "tiled " + row_name + " plus"; break;
                        case CullingMode::DISABLED: break;
                    }

                    if (render_mode == RenderMode::FORWARD && culling_mode == CullingMode::TILED && depth_mode == DepthMode::DISABLED) {
                        continue;
                    } else {
                        switch (depth_mode) {
                            case DepthMode::ENABLED:  row_name = row_name + " w/z pass"; break;
                            case DepthMode::DISABLED: break;
                        }                
                    }
                    switch (antialias) {
                        case AntiAlias::MSAA_2: row_name = row_name + " w/msaa2"; break;
                        case AntiAlias::MSAA_4: row_name = row_name + " w/msaa4"; break;
                        case AntiAlias::MSAA_8: row_name = row_name + " w/msaa8"; break;
                        case AntiAlias::NONE: break;
                    }

                    renderer.recreate();
                    test(samples);
                    log("res/logs/results.csv", row_name, samples);
                }
            }
        }
    }
}