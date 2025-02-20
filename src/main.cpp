#include "window.h"
#include "renderer.h"

/*
TODOLIST:
 * add signals to window, eg resize
 * connect signals to renderer eg resize -> recreate swapchain
 * materials
 * lights
 * forward renderer
 * deferred renderer
 * add imgui
 * settings menu
*/

int main() {
    Window window(800u, 600u, DisplayMode::WINDOWED);
    Renderer renderer(window, AntiAlias::NONE, VsyncMode::OFF, SyncMode::DOUBLE);
    while (!window.closed()) { 
        renderer.draw();
    }
}

