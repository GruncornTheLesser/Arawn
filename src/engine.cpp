#define ARAWN_INCLUDE_VULKAN
#include <engine.h>

using namespace arawn;

Engine::Engine(const core::Settings& info) {
	instance.create(info);
	window.create(instance, info);
	device.create(instance, window, info);
	swap.create(window, device, info);
}

Engine::~Engine() noexcept {
	swap.destroy(device);
	device.destroy(instance);
	window.destroy(instance);
	instance.destroy();
}

// TODO: implement engine move constructor
/*
Engine(Engine&& engine) noexcept;
Engine& operator=(Engine&& engine) noexcept;

Engine(const Engine& engine) noexcept;
Engine& operator=(const Engine& engine) noexcept;
*/

void Engine::configure(const core::Settings &value) {

}

bool Engine::closed() const {
	glfwPollEvents();
	
	return glfwWindowShouldClose(window.window);
}

void Engine::render() {
	frame.advance();
	
	auto& early   = frame.pass.earlyCull;
    auto& depth   = frame.pass.depthOnly;
    auto& late    = frame.pass.lateCull;
    auto& forward = frame.pass.forward;
    struct { VkCommandBuffer cmd; VkSemaphore finished; } post { 
		.cmd = render.postprocess[swapIndex],
		.finished = frame.frameReady
	};

	auto depthWaits = std::to_array<VkSemaphore>({ 
		early.finished
	});
	auto depthWaitDsts = std::to_array<VkPipelineStageFlags>({
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
	});
	auto lateWaits = std::to_array<VkSemaphore>({ 
		depth.finished
	});
	auto lateWaitDsts = std::to_array<VkPipelineStageFlags>({
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	});
	auto fwdWaits = std::to_array<VkSemaphore>({ 
		late.finished
	});
	auto fwdWaitDsts = std::to_array<VkPipelineStageFlags>({
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT // cluster mesh buffers
	});
	auto postWaits = std::to_array<VkSemaphore>({ 
		forward.finished,
	});
	auto postWaitDsts = std::to_array<VkPipelineStageFlags>({
		VK_PIPELINE_STAGE_TRANSFER_BIT
	});
	
	auto submits = std::to_array<VkSubmitInfo>({
		{ // early cull
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &early.cmd,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &early.finished,
		},
		{ // depth only
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = static_cast<uint32_t>(depthWaits.size()),
			.pWaitSemaphores = depthWaits.data(),
			.pWaitDstStageMask = depthWaitDsts.data(),
			.commandBufferCount = 1,
			.pCommandBuffers = &depth.cmd,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &depth.finished,
		},
		{ // late cull
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = static_cast<uint32_t>(lateWaits.size()),
			.pWaitSemaphores = lateWaits.data(),
			.pWaitDstStageMask = lateWaitDsts.data(),
			.commandBufferCount = 1,
			.pCommandBuffers = &late.cmd,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &late.finished,
		},
		{ // forward
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = static_cast<uint32_t>(fwdWaits.size()),
			.pWaitSemaphores = fwdWaits.data(),
			.pWaitDstStageMask = fwdWaitDsts.data(),
			.commandBufferCount = 1,
			.pCommandBuffers = &forward.cmd,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &forward.finished,
		},
		{ // post process
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = static_cast<uint32_t>(postWaits.size()),
			.pWaitSemaphores = postWaits.data(),
			.pWaitDstStageMask = postWaitDsts.data(),
			.commandBufferCount = 1,
			.pCommandBuffers = &post.cmd,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &post.finished,
		}
		
	});
	vkQueueSubmit(device.queue.compute.queue, 1, &submits[0], nullptr);  // early cull
	vkQueueSubmit(device.queue.graphics.queue, 1, &submits[1], nullptr); // depth only
	vkQueueSubmit(device.queue.compute.queue, 1, &submits[2], nullptr);  // late cull
	vkQueueSubmit(device.queue.graphics.queue, 2, &submits[3], nullptr); // forward + postprocess
	
	{
		VkPresentInfoKHR presentInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &frame.pass.forward.finished,
			.swapchainCount = 1,
			.pSwapchains = &swap.chain,
			.pImageIndices = &swapIndex
		};

		VkResult result = vkQueuePresentKHR(device.queue.present.queue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			
		} else {
			VK_ASSERT(result);
		}
	}

	render.index = (render.index + 1) % render.frames.size();
	*/
}