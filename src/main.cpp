
#include "engine.h"
#include "window.h"
#include "renderer.h"
#include <entt/entt.hpp>
#include <iostream>


/*
TODOLIST:
 * connect signals to renderer eg resize -> recreate swapchain
 * materials
 * lights
 * forward renderer
 * deferred renderer
 * add gui
 * settings menu
 * scene loading - https://casual-effects.com/data/
 * performance metrics
*/

const Engine engine{ "Arawn", "Arawn-vulkan", "NVIDIA GeForce RTX 3060 Laptop GPU" };

int main() {
    Renderer app(1280, 960, DisplayMode::WINDOWED, VsyncMode::ON, SyncMode::DOUBLE, AntiAlias::MSAA_4);

    // log app position
    //app.on<Mouse::Event>().attach([&](const Mouse::Event& event) { 
    //    std::cout << event.x << ", " << event.y << std::endl;
    //});
    
    // toggle fullscreen hotkey
    app.on<Key::Event>().attach([&](const Key::Event& event) { 
        if (event == Key::DOWN && event == Key::F) {
            auto curr_mode = app.get_display_mode();
            app.set_display_mode(static_cast<DisplayMode>((static_cast<uint32_t>(curr_mode) + 1) % 3));
            std::cout << "fullscreen set" << std::endl;
        }
    });
    
    // switch resolution
    size_t index = 0;
    auto resolutions = app.enum_resolutions(16.0f / 9.0f, 0.1f);
    app.on<Key::Event>().attach([&](const Key::Event& event) {
        if (event != Key::DOWN) return;
        
        if      (event == Key::PLUS) {
            if (index == resolutions.size() - 1) return;
            ++index;
        }
        else if (event == Key::MINUS) {
            if(index == 0) return;
            --index;
        }
        else return;
        
        glm::uvec2 res = resolutions[index];
        app.set_resolution(res.x, res.y);
        std::cout << "size set: " << res.x << ", " << res.y << std::endl;
    });
    
    

    
    while (!app.closed()) app.draw();
}
