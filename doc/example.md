```c++
void example(GLFW_WINDOW wnd) {
		Graph::Builder builder{ };

		uint32_t swap_attachment = builder.set_output({ wnd, 2, true, true });
		uint32_t color_attachment = builder.add_image({ .resolution = { 800, 600 }, .format = VK_FORMAT_R8G8B8A8_SNORM, .sample_count = VK_SAMPLE_COUNT_4_BIT });
		uint32_t depth_attachment = builder.add_image({ .resolution = { 800, 600 }, .format = VK_FORMAT_D32_SFLOAT,     .sample_count = VK_SAMPLE_COUNT_4_BIT });
		uint32_t trans_attachment = builder.add_image({ .resolution = { 800, 600 }, .format = VK_FORMAT_R8G8B8A8_SNORM, .sample_count = VK_SAMPLE_COUNT_4_BIT });
		uint32_t blur_attachment =  builder.add_image({ .resolution = { 800, 600 }, .format = VK_FORMAT_R8G8B8A8_SNORM, .levels = 8 }); // mipmaps

		uint32_t cull_indices_buffer = builder.add_buffer({ });
		uint32_t light_buffer        = builder.add_buffer({ });

		builder.add_task({
			.vert_shader = "shader/transform.vert", .frag_shader = "shader/depth.frag",
			.resources = {
				{ .handle = depth_attachment, .access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, .image = { .final_layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL } }
			},
			.record = [=](const Context& ctx, const Scene& scn) {
				// scene.visit([&](Transparent& model){ model.draw(context); });
			}
		});
	
		builder.add_task({
			.comp_shader = "shader/culling.comp", 
			.resources = {
				{ .handle = depth_attachment,    .access = VK_ACCESS_SHADER_READ_BIT,  },
				{ .handle = cull_indices_buffer, .access = VK_ACCESS_SHADER_WRITE_BIT, },
				{ .handle = light_buffer,        .access = VK_ACCESS_SHADER_WRITE_BIT, },
			},
			.record = [=](const Context& ctx, const Scene& scn) {
				// scene.visit([&](Transparent& model){ model.draw(context); });
			}
		});
	
		builder.add_task({
			.vert_shader = "shader/transform.vert", .frag_shader = "shader/transparent.frag",
			.resources = { 
				{ .handle = color_attachment, .access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT         }, 
				{ .handle = depth_attachment, .access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT }, 
				{ .handle = trans_attachment, .access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT        }, 
			},
			.record = [=](const Context& ctx, const Scene& scn) {
				// scene.visit([&](Transparent& model){ model.draw(context); });
			}
		});
	
		builder.add_task({
			.vert_shader = "shader/transform.vert", .frag_shader = "shader/opaque.frag",
			.resources = {
				{ .handle = cull_indices_buffer, .access = VK_ACCESS_SHADER_READ_BIT                    },
				{ .handle = light_buffer,        .access = VK_ACCESS_SHADER_READ_BIT                    },
				{ .handle = color_attachment,    .access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT         }, 
				{ .handle = depth_attachment,    .access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, },
			},
			.record = [=](const Context& ctx, const Scene& scn) {
				// scene.visit([&](Transparent& model){ model.draw(context); });
			}
		});
		
		builder.add_task({ 
			.vert_shader = "", .frag_shader = "", // no program
			.resources = { 
				{ .handle = color_attachment, .access = VK_ACCESS_TRANSFER_READ_BIT }, 
				{ .handle = swap_attachment, .access = VK_ACCESS_TRANSFER_WRITE_BIT }
			},
			.record = [=](const Context& context, const Scene& scene) {
				// blit to swapchain image
				const Resource::Image& src_img = context.get_image(color_attachment);
				const Resource::Image& dst_img = context.get_image(swap_attachment);
	
				VkImageBlit regions {
					.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }, .srcOffsets = { { 0, 0, 0 }, { src_img.info.resolution, 1 } },
					.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }, .dstOffsets = { { 0, 0, 0 }, { dst_img.info.resolution, 1 } },
				};
	
				vkCmdBlitImage(context.cmd, src_img.image, src_img.layout, dst_img.image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, &regions, VK_FILTER_LINEAR);
			}
		});
	}

```
