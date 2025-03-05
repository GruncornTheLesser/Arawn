#define VK_IMPLEMENTATION
#include "renderer.h"
#include "engine.h"

Renderer::Renderer() {
    { // init descriptor set layouts
        VkDescriptorSetLayoutBinding camera_binding;
        camera_binding.binding = 0;
        camera_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        camera_binding.descriptorCount = 1;
        camera_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        camera_binding.pImmutableSamplers = nullptr;
        
        VkDescriptorSetLayoutBinding transform_binding;
        transform_binding.binding = 0;
        transform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        transform_binding.descriptorCount = 1;
        transform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        transform_binding.pImmutableSamplers = nullptr;


        VkDescriptorSetLayoutBinding light_binding;
        light_binding.binding = 0;
        light_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        light_binding.descriptorCount = 1;
        light_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        light_binding.pImmutableSamplers = nullptr;
        
        VkDescriptorSetLayoutBinding frustrums_binding;
        frustrums_binding.binding = 1;
		frustrums_binding.binding = 1;
		frustrums_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		frustrums_binding.descriptorCount = 1;
		frustrums_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		frustrums_binding.pImmutableSamplers = nullptr;       
    }
}

void Renderer::draw() {
    const uint64_t timeout = 1000000000; // 1 second

    uint32_t image_index;
    VkFence in_flight = swapchain.in_flight_fence[swapchain.frame_index];
    VkSemaphore image_available = swapchain.image_available_semaphore[swapchain.frame_index];
    VkCommandBuffer cmd_buffer = swapchain.cmd_buffers[swapchain.frame_index];
    VkSemaphore render_finished = swapchain.render_finished_semaphore[swapchain.frame_index];

    { // get next swapchain image
        VK_ASSERT(vkWaitForFences(engine.device, 1, &in_flight, true, timeout));
        
        VkResult res = vkAcquireNextImageKHR(engine.device, swapchain.swapchain, timeout, image_available, nullptr, &image_index);

        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            //recreate_swapchain();
            return;
        } else if (res != VK_SUBOPTIMAL_KHR) {
            VK_ASSERT(res);
        }
        
    }
    
    // update ubos
    
    VK_ASSERT(vkResetFences(engine.device, 1, &in_flight));

    // present cmd buffer

    // submit command buffers
    { // depth pre pass
        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 0;
        info.pWaitSemaphores = nullptr;
        info.pWaitDstStageMask = nullptr;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &depth_pass.cmd_buffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &depth_pass.finished;
        
        vkQueueSubmit(engine.graphics.queue, 1, &info, nullptr);
    }

    { // cull pass
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &depth_pass.finished;
        info.pWaitDstStageMask = wait_stages;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &culling_pass.cmd_buffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &culling_pass.finished;

        vkQueueSubmit(engine.compute.queue, 1, &info, nullptr);
    }

    { // present pass
        VkSemaphore wait_semaphores[] = { image_available, culling_pass.finished };
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 2;
        info.pWaitSemaphores = wait_semaphores;
        info.pWaitDstStageMask = wait_stages;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &forward_pass.cmd_buffer;
    }

    { // present to screen
        VkSemaphore wait[1] = { render_finished };

        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;;
        info.pNext = nullptr;
        info.waitSemaphoreCount = 1; // !!!!
        info.pWaitSemaphores = wait;
        info.swapchainCount = 1;
        info.pSwapchains = &swapchain.swapchain;
        info.pImageIndices = &image_index;
        info.pResults = nullptr;
        
        VkResult res = vkQueuePresentKHR(engine.present.queue, &info);

        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            //recreate_swapchain();
        } else {
            VK_ASSERT(res);
        }
    }

    swapchain.frame_index = (swapchain.frame_index + 1) % swapchain.frame_count; // get next frame data set
}

// pipeline, layout and shader loading
/*
auto create_shader_module(std::filesystem::path fp) -> VkShaderModule {
    std::ifstream file(fp, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("could not open file");

    std::vector<char> buffer(static_cast<std::size_t>(file.tellg()));
    file.seekg(0);
    file.read(buffer.data(), buffer.size());
    file.close();

    VkShaderModuleCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.pCode = reinterpret_cast<uint32_t*>(buffer.data());
    info.codeSize = buffer.size();

    VkShaderModule shader;
    VK_ASSERT(vkCreateShaderModule(engine.device, &info, NULL, &shader));
    return shader;
}

auto init_pipeline_layout(VkDescriptorSetLayout set_layout)
 -> VkPipelineLayout {
    
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.pushConstantRangeCount = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr; //&set_layout;

    VkPipelineLayout pipeline_layout;
    VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &pipeline_layout));
    return pipeline_layout;
}

auto init_pipeline(VkRenderPass renderpass, VkPipelineLayout layout, VkSampleCountFlagBits msaa_sample, VkShaderModule vert, VkShaderModule frag)
 -> VkPipeline {
    VkPipelineShaderStageCreateInfo shader_stages[2];
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].pNext = nullptr;
    shader_stages[0].flags = 0;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vert;
    shader_stages[0].pName = "main";
    shader_stages[0].pSpecializationInfo = nullptr;

    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].pNext = nullptr;
    shader_stages[1].flags = 0;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = frag;
    shader_stages[1].pName = "main";
    shader_stages[1].pSpecializationInfo = nullptr;

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 0;
    vertex_input.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // COUNTER_
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = msaa_sample;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState resolve_attachment{};
    resolve_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    resolve_attachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colour_blending{};
    colour_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colour_blending.logicOpEnable = VK_FALSE;
    colour_blending.logicOp = VK_LOGIC_OP_COPY;
    colour_blending.attachmentCount = 1;
    colour_blending.pAttachments = &resolve_attachment;
    colour_blending.blendConstants[0] = 0.0f;
    colour_blending.blendConstants[1] = 0.0f;
    colour_blending.blendConstants[2] = 0.0f;
    colour_blending.blendConstants[3] = 0.0f;

    VkDynamicState dynamic_states[] { 
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR 
    };
    
    VkPipelineDynamicStateCreateInfo dynamics{};
    dynamics.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamics.dynamicStateCount = 2;
    dynamics.pDynamicStates = dynamic_states;

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = 2;
    info.pStages = shader_stages;
    info.pVertexInputState = &vertex_input;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport_state;
    info.pRasterizationState = &rasterizer;
    info.pMultisampleState = &multisampling;
    info.pDepthStencilState = &depth_stencil;
    info.pColorBlendState = &colour_blending;
    info.pDynamicState = &dynamics;
    info.layout = layout;
    info.renderPass = renderpass;
    info.subpass = 0;
    info.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline;
    VK_ASSERT(vkCreateGraphicsPipelines(engine.device, nullptr, 1, &info, nullptr, &pipeline));

    return pipeline;
}

*/

// swapchain draw 
/*
void Swapchain::draw() {
    const uint64_t timeout = 1000000000; // 1 second
    
    uint32_t image_index;
    VkFence in_flight = in_flight_fence[frame_index];
    VkSemaphore image_available = image_available_semaphore[frame_index];
    VkCommandBuffer cmd_buffer = cmd_buffers[frame_index];
    VkSemaphore render_finished = render_finished_semaphore[frame_index];
    
    { // get next swapchain image
        VK_ASSERT(vkWaitForFences(engine.device, 1, &in_flight, true, timeout));
        
        VkResult res = vkAcquireNextImageKHR(engine.device, swapchain, timeout, image_available, nullptr, &image_index);

        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swapchain();
            return;
        } else if (res != VK_SUBOPTIMAL_KHR) {
            VK_ASSERT(res);
        }
    }
    
    { // update camera uniform buffer

    }

    { // update light storage buffer

    }

    VK_ASSERT(vkResetFences(engine.device, 1, &in_flight));
    VK_ASSERT(vkResetCommandBuffer(cmd_buffer, 0));
    
    { // record cmd buffer
        { // begin command buffer
            VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
            VK_ASSERT(vkBeginCommandBuffer(cmd_buffer, &info));
        }

        { // begin renderpass
            VkClearValue clear_value[3];
            clear_value[0].depthStencil = { 1.0f, 0 };
            clear_value[1].color = {{ 0.58f, 0, 0.84, 0 }};
            clear_value[2].color = {{ 0.58f, 0, 0.84, 0 }};

            VkRenderPassBeginInfo info{};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.pNext = nullptr;
            info.renderPass = renderpass;
            info.framebuffer = framebuffers[image_index];
            info.renderArea.offset = { 0, 0 };
            info.renderArea.extent = { settings.resolution.x, settings.resolution.y };
            info.clearValueCount = settings.anti_alias == AntiAlias::NONE ? 2 : 3;
            info.pClearValues = clear_value;
            
            vkCmdBeginRenderPass(cmd_buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        }
        
        { // dynamics
            vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            
            { // set viewport
                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = static_cast<float>(settings.resolution.x);
                viewport.height = static_cast<float>(settings.resolution.y);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
            }
            { // set scissors 
                VkRect2D scissor{ };
                scissor.offset = {0, 0};
                scissor.extent = { settings.resolution.x , settings.resolution.y };
                vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
            }

            vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vertex_buffer, 0);

            vkCmdBindIndexBuffer(cmd_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmd_buffer, indices.size(), 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(cmd_buffer);
        
        VK_ASSERT(vkEndCommandBuffer(cmd_buffer));
    }

    { // submit cmd buffer
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.pNext = nullptr;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_available; // wait for next image to be available
        info.pWaitDstStageMask = wait_stages;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &cmd_buffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_finished;

        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, in_flight));
    }

    { // present to screen
        VkSemaphore wait[1] = { render_finished };

        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;;
        info.pNext = nullptr;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = wait;
        info.swapchainCount = 1;
        info.pSwapchains = &swapchain;
        info.pImageIndices = &image_index;
        info.pResults = nullptr;
        
        VkResult res = vkQueuePresentKHR(engine.present.queue, &info);

        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            recreate_swapchain();
        } else {
            VK_ASSERT(res);
        }
    }

    frame_index = (frame_index + 1) % frame_count; // get next frame data set
}
*/

// recreate swapchain renderpass and attachments
/*
    { // init renderpass
        VkAttachmentDescription attachments[3];
        // depth attachment
        attachments[0].flags = 0;
        attachments[0].format = depth_format;
        attachments[0].samples = sample_count;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // swapchain image attachment -> if msaa enabled used as colour attachment, else used as resolve attachment
        attachments[1].flags = 0;
        attachments[1].format = colour_format;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // colour attachment
        attachments[2].flags = 0;
        attachments[2].format = colour_format;
        attachments[2].samples = sample_count;
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkAttachmentReference dep_refs[1]; // depth attachment
        dep_refs[0] = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
        
        VkAttachmentReference res_refs[1]; // swapchain image attachment
        res_refs[0] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        
        VkAttachmentReference col_refs[1]; // colour attachment(if needed)
        col_refs[0] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        
        VkSubpassDescription subpasses[1];
        subpasses[0].flags = 0;
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].inputAttachmentCount = 0;
        subpasses[0].pInputAttachments = nullptr;
        subpasses[0].colorAttachmentCount = 1;
        subpasses[0].pDepthStencilAttachment = dep_refs;
        subpasses[0].preserveAttachmentCount = 0;
        subpasses[0].pPreserveAttachments = nullptr;
        if (msaa_enabled) {
            subpasses[0].pColorAttachments = col_refs;
            subpasses[0].pResolveAttachments = res_refs;
        }
        else {
            subpasses[0].pColorAttachments = res_refs;
            subpasses[0].pResolveAttachments = nullptr;
        }

        VkSubpassDependency dependencies[1];
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = 0;
        

        
        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.attachmentCount = attachment_count;
        info.pAttachments = attachments;
        info.subpassCount = 1;
        info.pSubpasses = subpasses;
        info.dependencyCount = 1;
        info.pDependencies = dependencies;

        VK_ASSERT(vkCreateRenderPass(engine.device, &info, nullptr, &renderpass));
    }

    { // init depth attachment
        { // init image
            VkImageCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.extent = { settings.resolution.x, settings.resolution.y, 1 };
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.format = depth_format;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;   // !!!
            info.samples = sample_count;                                // !!!
            info.sharingMode = engine.graphics.family == engine.present.family ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT; // !!!

            VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &depth_attachment.image));
        }

        { // init memory
            VkMemoryRequirements requirements;
            vkGetImageMemoryRequirements(engine.device, depth_attachment.image, &requirements);
            uint32_t memory_index = engine.get_memory_index(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VkMemoryAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            info.pNext = nullptr;
            info.memoryTypeIndex = memory_index;
            info.allocationSize = requirements.size;

            VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &depth_attachment.memory));
        }
        
        VK_ASSERT(vkBindImageMemory(engine.device, depth_attachment.image, depth_attachment.memory, 0));

        { // init view
            VkImageViewCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.image = depth_attachment.image;
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = depth_format;
            info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = 1;

            VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &depth_attachment.view));
        }
    }
    
    auto image_sharing_mode = engine.graphics.family == engine.present.family ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
    if (msaa_enabled) { // init colour attachment
        { // init image
            VkImageCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.extent = { settings.resolution.x, settings.resolution.y, 1 };
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.format = colour_format;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            info.samples = sample_count;
            info.sharingMode = engine.graphics.family == engine.present.family ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

            VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &resolve_attachment.image));
        }

        { // init memory
            VkMemoryRequirements requirements;
            vkGetImageMemoryRequirements(engine.device, resolve_attachment.image, &requirements);
            uint32_t memory_index = engine.get_memory_index(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VkMemoryAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            info.pNext = nullptr;
            info.memoryTypeIndex = memory_index;
            info.allocationSize = requirements.size;

            VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &resolve_attachment.memory));
        }
        
        VK_ASSERT(vkBindImageMemory(engine.device, resolve_attachment.image, resolve_attachment.memory, 0));

        { // init view
            VkImageViewCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.image = resolve_attachment.image;
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = colour_format;
            info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = 1;

            VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &resolve_attachment.view));
        }
    } else {
        resolve_attachment.image = nullptr;
        resolve_attachment.memory = nullptr;
        resolve_attachment.view = nullptr;
    }

    { // init framebuffers
        uint32_t image_count;
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr));
        
        framebuffers = std::allocator<VkFramebuffer>().allocate(image_count);
        
        VkImageView attachments[3] { depth_attachment.view, nullptr, resolve_attachment.view };
        //                                                                  ^ swapchain image view

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.renderPass = renderpass;
        info.attachmentCount = attachment_count;
        info.pAttachments = attachments;
        info.width = settings.resolution.x;
        info.height = settings.resolution.y;
        info.layers = 1;

        for (uint32_t i = 0; i < image_count; ++i) {
            attachments[1] = swapchain_views[i];
            VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &framebuffers[i]));
        }
    }
    */