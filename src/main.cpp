#include "engine.h"
#include "window.h"
#include "swapchain.h"
#include "model.h"
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
// Renderer    renderer;
int main() {
    Model       model("res/model/bunny.obj");
    return 0;
    
    // toggle fullscreen hotkey
    window.on(KeyDown{ Key::F }) += [&](auto& event) {
        window.set_display_mode(static_cast<DisplayMode>((static_cast<uint32_t>(window.get_display_mode()) + 1) % 3));
    };
    
    // switch resolution
    size_t index = 0;
    auto resolutions = window.enum_resolutions(4.0f / 3.0f);
    
    window.on(KeyDown{ Key::PLUS }) += [&](auto& event) { // increase resolution
        if (index == resolutions.size() - 1) return;
        window.set_resolution(resolutions[++index]);
    };

    window.on(KeyDown{ Key::MINUS }) += [&](auto& event) { // decrease resolution
        if(index == 0) return;
        window.set_resolution(resolutions[--index]);
    };
    
    // while (!window.closed()) swapchain.draw();
}
