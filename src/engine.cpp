#define ARAWN_INCLUDE_VULKAN
#include <engine.h>

using namespace arawn;

Engine::Engine(const Settings& info) {
	core.create(info);
	window.create(core, info);
	device.create(core, window, info);
	world.create(device, info);
	swap.create(core, window, device, info);
	// TODO: vvv
	render.create(core, window, device, world, info); 
	//present.create(core, window, device, info);
}

Engine::~Engine() noexcept {
	// present.destroy(core, device);
	render.destroy(core, device);
	swap.destroy(core, device);
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


bool Engine::closed() const {
	glfwPollEvents();

	// render here





	return glfwWindowShouldClose(window.window);
}


