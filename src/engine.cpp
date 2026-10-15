#define ARAWN_INCLUDE_VULKAN
#include <engine.h>

using namespace arawn;

Engine::Engine(const Settings& info) {
	core.create(info);
	window.create(core, info);
	device.create(core, window, info);
	swap.create(window, device, info);
	world.create(device, info);
	render.create(window, device, world, swap, info); 
	//present.create(core, window, device, info);
}

Engine::~Engine() noexcept {
	render.destroy(device);
	swap.destroy(device);
	world.destroy(device);
	device.destroy(core);
	window.destroy(core);
	core.destroy();
}

// TODO: implement engine move constructor
/*
Engine(Engine&& engine) noexcept;
Engine& operator=(Engine&& engine) noexcept;

Engine(const Engine& engine) noexcept;
Engine& operator=(const Engine& engine) noexcept;
*/

void Engine::configure(const Settings &value) {

}

bool Engine::closed() const {
	glfwPollEvents();

	// render here





	return glfwWindowShouldClose(window.window);
}

void Engine::process() {

}