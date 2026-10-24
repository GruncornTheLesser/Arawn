#define ARAWN_INCLUDE_VULKAN
#include <engine.h>

using namespace arawn;

Engine::Engine(const core::Settings& set) : settings(set) {
	// core
	instance.create(settings);
	window.create(instance, settings);
	device.create(instance, window, settings);
	
	// domains
	swap.create(window, device, settings);
	frame.create(device, settings);

	// pools
	pool.node.create(device, settings);
	// pool.mesh.create(device, settings);
	pool.light.create(device, settings);
	// pool.texture.create(device, settings);
	pool.material.create(device, settings);

	// attachments
	attachment.color.create(device, frame, settings);
	attachment.depth.create(device, frame, settings);
	attachment.ztree.create(device, frame, settings);
	// attachment.cull.mesh.create(device, frame, settings);
	// attachment.cull.light.create(device, frame, settings);

	// pipelines
	pipeline.cull.frustum.create(device);
	pipeline.cull.occlusion.create(device);
	pipeline.cull.light.create(device);
	pipeline.cull.mesh.create(device);
	pipeline.gen.ztree.create(device);
	pipeline.gen.tile.create(device);
	pipeline.gen.cluster.create(device);
	pipeline.depthOnly.create(device, attachment.depth);
	pipeline.forward.create(device, attachment.color, attachment.depth);

	// passes // TODO: unfinished
	// pass.earlycull.create(device);
	// pass.depthonly.create(device);
	// pass.latecull.create(device);
	// pass.forward.create(device);
	// pass.present.create(device);
}

Engine::~Engine() noexcept {
	// passes
	// pass.earlycull.destroy(device);
	// pass.depthonly.destroy(device);
	// pass.latecull.destroy(device);
	// pass.forward.destroy(device);
	// pass.present.destroy(device);
	
	// attachments
	attachment.color.destroy(device);
	attachment.depth.destroy(device);
	attachment.ztree.destroy(device);
	// attachment.cull.mesh.destroy(device, frame, settings);
	// attachment.cull.light.destroy(device, frame, settings);
	
	// pools
	pool.node.destroy(device);
	// pool.mesh.destroy(device);
	pool.light.destroy(device);
	// pool.texture.destroy(device);
	pool.material.destroy(device);

	// pipeline
	pipeline.cull.frustum.destroy(device);
	pipeline.cull.occlusion.destroy(device);
	pipeline.cull.light.destroy(device);
	pipeline.cull.mesh.destroy(device);
	pipeline.gen.ztree.destroy(device);
	pipeline.gen.tile.destroy(device);
	pipeline.gen.cluster.destroy(device);
	pipeline.depthOnly.destroy(device);
	pipeline.forward.destroy(device);

	// domain
	frame.destroy(device);
	swap.destroy(device);
	
	// core
	device.destroy(instance);
	window.destroy(instance);
	instance.destroy();
}

void Engine::configure(const core::Settings &value) {
	// instance reconstruction - hell no

	// device reconstruction - want
	//  - needs to give every resource a reconstruction strategy
	// resource.budget - hell no
	//  - recreate all pools, 
	//  - copy resources from old buffer to new

	// frame - must
	//  - resize and create/destroy each framed resource

	// display
	//  - might be able to implement this lazily
	//  - rely on acquire frame vulkan error code to recreate swapchain

	// window/swap - must
	//  - recreate image attachments
	//  - recreate tile and cluster buffers sizes

	
	
}

bool Engine::closed() const {
	glfwPollEvents();
	
	return glfwWindowShouldClose(window.window);
}

void Engine::render() {
	frame.advance();

	if (VkResult res = swap.advance(device, frame); res == VK_ERROR_OUT_OF_DATE_KHR) {
		ARAWN_LOG(INFO, "surface image out of date, recreating renderer");
		// TODO: recreate
	} else if (res != VK_SUBOPTIMAL_KHR) {
		VK_ASSERT(res);
	}

	/*
	pool.node.upload(device);
	pool.mesh.upload(device);
	pool.light.upload(device);
	pool.texture.upload(device);
	pool.material.upload(device);

	pass.earlycull.submit(device, frame);
	pass.depthonly.submit(device, frame, pass.earlycull);
	pass.latecull.submit(device, frame, pass.depthonly);
	pass.forward.submit(device, frame, pass.latecull);
	pass.present.submit(device, frame, swap, pass.forward);
	*/
}

/*
	auto& frame = render.frames[render.index];

	const uint64_t timeout = 1000000000;
	VK_ASSERT(vkWaitForFences(device.device, 1, &frame.inFlight, true, timeout));
	
	uint32_t swapIndex;
	VkResult result = vkAcquireNextImageKHR(device.device, swap.chain, timeout, frame.imageReady, nullptr, &swapIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		// TODO: recreate
		ARAWN_LOG(INFO, "surface image out of date, recreating renderer");
		return;
	} else if (result != VK_SUBOPTIMAL_KHR) {
		VK_ASSERT(result);
	}

	VK_ASSERT(vkResetFences(device.device, 1, &frame.inFlight));

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