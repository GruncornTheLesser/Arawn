
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

const Engine engine{ "ARAWN", "ARAWN-vulkan" };

int main() {
    Window window(640, 400);
    window.on<Mouse::Event>().attach([](auto& event) { 
        std::cout << event.x << ", " << event.y << std::endl;
    });

    Renderer renderer(window, AntiAlias::MSAA_4, VsyncMode::ON, SyncMode::DOUBLE);
    while (!window.closed()) renderer.draw();
}
