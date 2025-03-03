#define VK_IMPLEMENTATION
#include "swapchain.h"
#include "engine.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
auto init_pipeline_layout(VkDescriptorSetLayout set_layout) -> VkPipelineLayout; 
auto init_pipeline(VkRenderPass renderpass, VkPipelineLayout layout, VkSampleCountFlagBits msaa_sample, VkShaderModule vert, VkShaderModule frag) -> VkPipeline;

Swapchain::Swapchain()
{
    // create surface
    VK_ASSERT(glfwCreateWindowSurface(engine.instance, window.window, nullptr, &surface));

    swapchain = nullptr;
    recreate_swapchain();

    { // init descriptor set layout
        VkDescriptorSetLayoutBinding bindings[3];
        bindings[0].binding = 0; // lights
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        bindings[1].binding = 1; // materials
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        bindings[2].binding = 2; // materials images
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[2].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.bindingCount = 3;
        info.pBindings = bindings;

        VK_ASSERT(vkCreateDescriptorSetLayout(engine.device, &info, nullptr, &set_layout));
    }

    auto vert = create_shader_module("res/import/shaders/def.vert.spv");
    auto frag = create_shader_module("res/import/shaders/def.frag.spv");
    
    layout = init_pipeline_layout(set_layout);
    pipeline = init_pipeline(renderpass, layout, static_cast<VkSampleCountFlagBits>(settings.anti_alias), vert, frag);
    
    vkDestroyShaderModule(engine.device, vert, nullptr);
    vkDestroyShaderModule(engine.device, frag, nullptr);
}

Swapchain::~Swapchain() {
    vkDeviceWaitIdle(engine.device);

    vkDestroyPipeline(engine.device, pipeline, nullptr);
    vkDestroyPipelineLayout(engine.device, layout, nullptr);
    
    vkDestroyDescriptorSetLayout(engine.device, set_layout, nullptr);

    vkFreeCommandBuffers(engine.device, engine.graphics.pool, frame_count, cmd_buffers);
    for (uint32_t i = 0; i < frame_count; ++i) {
        vkDestroySemaphore(engine.device, render_finished_semaphore[i], nullptr);
        vkDestroySemaphore(engine.device, image_available_semaphore[i], nullptr);
        vkDestroyFence(engine.device, in_flight_fence[i], nullptr);
    }

    bool old_msaa_enabled = resolve_attachment.image != nullptr;
    
    uint32_t image_count;
    vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr);

    for (uint32_t i = 0; i < image_count; ++i)
        vkDestroyFramebuffer(engine.device, framebuffers[i], nullptr);
    std::allocator<VkFramebuffer>().deallocate(framebuffers, image_count);

    // destroy depth attachment
    vkDestroyImageView(engine.device, depth_attachment.view, nullptr);
    vkFreeMemory(engine.device, depth_attachment.memory, nullptr);
    vkDestroyImage(engine.device, depth_attachment.image, nullptr);
    
    if (old_msaa_enabled) { // destroy resolve attachment
        vkDestroyImageView(engine.device, resolve_attachment.view, nullptr);
        vkFreeMemory(engine.device, resolve_attachment.memory, nullptr);
        vkDestroyImage(engine.device, resolve_attachment.image, nullptr);
    }
    
    vkDestroyRenderPass(engine.device, renderpass, nullptr);
    
    for (uint32_t i = 0; i < image_count; ++i) 
        vkDestroyImageView(engine.device, swapchain_views[i], nullptr);
    std::allocator<VkImageView>().deallocate(swapchain_views, image_count);
    
    vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
 
    vkDestroySurfaceKHR(engine.instance, surface, nullptr);
}

void Swapchain::set_vsync_mode(VsyncMode mode) { 
    settings.vsync_mode = mode;
    recreate_swapchain();
}

std::vector<VsyncMode> Swapchain::enum_vsync_modes() const {
    std::vector<VsyncMode> vsync_modes;

    vsync_modes.push_back(VsyncMode::ON); // guaranteed to be supported VK_PRESENT_MODE_FIFO_KHR

    uint32_t count;
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
    std::vector<VkPresentModeKHR> supported(count);
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));

    if (std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != supported.end() || 
        std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end())
    {
        vsync_modes.push_back(VsyncMode::OFF);
    }

    return vsync_modes;   
}

void Swapchain::set_sync_mode(SyncMode mode) {
    frame_count = static_cast<uint32_t>(mode);
    recreate_swapchain();
}

std::vector<SyncMode> Swapchain::enum_sync_modes() const {
    std::vector<SyncMode> sync_modes;
    sync_modes.push_back(SyncMode::DOUBLE);
    
    uint32_t count;
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
    std::vector<VkPresentModeKHR> supported(count);
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));
    
    return sync_modes;
}

void Swapchain::set_anti_alias(AntiAlias mode) {
    settings.anti_alias = mode;
    recreate_swapchain();
    throw std::exception();
}

std::vector<AntiAlias> Swapchain::enum_anti_alias() const {
    std::vector<AntiAlias> anti_alias_modes = { AntiAlias::NONE }; // AntiAlias::FXAA_2, AntiAlias::FXAA_4, AntiAlias::FXAA_8, AntiAlias::FXAA_16
    
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(engine.gpu, &properties);

    if (properties.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_2_BIT)
        anti_alias_modes.push_back(AntiAlias::MSAA_2);

    if (properties.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_4_BIT)
        anti_alias_modes.push_back(AntiAlias::MSAA_4);

    if (properties.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_8_BIT)
        anti_alias_modes.push_back(AntiAlias::MSAA_8);

    if (properties.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_16_BIT)
        anti_alias_modes.push_back(AntiAlias::MSAA_16);

    return anti_alias_modes;
}

void Swapchain::draw() {
    const uint64_t timeout = 1000000000;
    
    uint32_t image_index;
    VkFence in_flight = in_flight_fence[frame_index];
    VkSemaphore image_available = image_available_semaphore[frame_index];
    
    { // get next swapchain image
        VK_ASSERT(vkWaitForFences(engine.device, 1, &in_flight, true, timeout));
        
        VkResult res = vkAcquireNextImageKHR(engine.device, swapchain, timeout, image_available, nullptr, &image_index);

        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swapchain();
            return;
        } else if (res != VK_SUBOPTIMAL_KHR) {
            VK_ASSERT(res);
        }
        
        VK_ASSERT(vkResetFences(engine.device, 1, &in_flight));
    }

    // TODO: update uniform buffers
    
    VkCommandBuffer cmd_buffer = cmd_buffers[frame_index];
    VkSemaphore render_finished = render_finished_semaphore[frame_index];
    
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
        
        { // draw
            vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            { // set viewport
                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = (float) settings.resolution.x;
                viewport.height = (float) settings.resolution.y;
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
            vkCmdDraw(cmd_buffer, 3, 1, 0, 0);
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

void Swapchain::recreate_swapchain() {
    while (window.minimized()) { glfwWaitEvents(); }
    
    VkSwapchainKHR old_swapchain = swapchain;   
    VkPresentModeKHR present_mode;
    VkFormat colour_format;
    VkColorSpaceKHR colour_space;
    VkFormat depth_format = engine.format.attachment.depth;
    VkSampleCountFlagBits sample_count = static_cast<VkSampleCountFlagBits>(settings.anti_alias);
    
    VkSurfaceCapabilitiesKHR capabilities;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine.gpu, surface, &capabilities));
    { // get swapchain extent
        if (capabilities.currentExtent.width == 0xffffffff) {
            settings.resolution = window.get_resolution();
        } 
        else {
            settings.resolution = glm::clamp(
                glm::uvec2(capabilities.currentExtent.width, capabilities.currentExtent.height), 
                glm::uvec2(capabilities.minImageExtent.width, capabilities.minImageExtent.height), 
                glm::uvec2(capabilities.maxImageExtent.width, capabilities.maxImageExtent.height));
        }
    }

    
    if (capabilities.minImageCount > frame_count) { // init additional cmd buffers if necessary
        VkCommandBufferAllocateInfo cmd_buffer_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, engine.graphics.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, capabilities.minImageCount - frame_count };
        VK_ASSERT(vkAllocateCommandBuffers(engine.device, &cmd_buffer_info, cmd_buffers + frame_count));
        
        VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
        VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
        for (uint32_t i = frame_count; i < capabilities.minImageCount; ++i) {
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore_info, nullptr, &render_finished_semaphore[i]));
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore_info, nullptr, &image_available_semaphore[i]));
            VK_ASSERT(vkCreateFence(engine.device, &fence_info, nullptr, &in_flight_fence[i]));
        }
        frame_count = capabilities.minImageCount;
    }

    { // get present mode
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
        std::vector<VkPresentModeKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));

        bool frame_skip_support; // frame skip allows the swapchain to present the most recently created image
        switch (settings.vsync_mode) {
        case VsyncMode::OFF:
            frame_skip_support = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end();
            present_mode = frame_skip_support ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        case VsyncMode::ON:
            frame_skip_support = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_MAILBOX_KHR) != supported.end();
            present_mode = frame_skip_support ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
            break;
        }
    }

    { // get surface colour format
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, nullptr));
        std::vector<VkSurfaceFormatKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, supported.data()));

        for (VkSurfaceFormatKHR& surface_format : supported) {
            switch (surface_format.format) {
                case(VK_FORMAT_R8G8B8A8_SRGB): break;
                case(VK_FORMAT_B8G8R8A8_SRGB): break;
                case(VK_FORMAT_R8G8B8A8_UNORM): break;
                case(VK_FORMAT_B8G8R8A8_UNORM): break;
                default: continue;
            }
            colour_format = surface_format.format;
            colour_space = surface_format.colorSpace;
        }
    }

    bool msaa_enabled = sample_count != VK_SAMPLE_COUNT_1_BIT;
    uint32_t attachment_count = msaa_enabled ? 3 : 2;

    { // init swapchain
        VkSwapchainCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.pNext = nullptr;
        info.flags = 0;
        info.surface = surface;
        info.minImageCount = frame_count;
        info.imageFormat = colour_format;
        info.imageColorSpace = colour_space;
        info.imageExtent = { settings.resolution.x, settings.resolution.y };
        info.imageArrayLayers = 1;                                  // 1 unless VR
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;      // present is copy operation from render image
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;          // images only accessed by present queue
        info.queueFamilyIndexCount = 1;
        info.pQueueFamilyIndices = &engine.present.family;
        info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;  // can rotate screen etc, normally for mobile
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;    // blend with other windows or not
        info.presentMode = present_mode;
        info.clipped = VK_TRUE;                                     // skip draw to obscured pixels
        info.oldSwapchain = old_swapchain;

        VK_ASSERT(vkCreateSwapchainKHR(engine.device, &info, nullptr, &swapchain));
    }

    // destroy old swapchain
    if (old_swapchain != nullptr) {
        VK_ASSERT(vkDeviceWaitIdle(engine.device));

        bool old_msaa_enabled = resolve_attachment.image != nullptr;
    
        uint32_t image_count;
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, old_swapchain, &image_count, nullptr));

        for (uint32_t i = 0; i < image_count; ++i)
            vkDestroyFramebuffer(engine.device, framebuffers[i], nullptr);
        std::allocator<VkFramebuffer>().deallocate(framebuffers, image_count);

        // destroy depth attachment
        vkDestroyImageView(engine.device, depth_attachment.view, nullptr);
        vkFreeMemory(engine.device, depth_attachment.memory, nullptr);
        vkDestroyImage(engine.device, depth_attachment.image, nullptr);
        
        if (old_msaa_enabled) { // destroy resolve attachment
            vkDestroyImageView(engine.device, resolve_attachment.view, nullptr);
            vkFreeMemory(engine.device, resolve_attachment.memory, nullptr);
            vkDestroyImage(engine.device, resolve_attachment.image, nullptr);
        }
        
        vkDestroyRenderPass(engine.device, renderpass, nullptr);
        
        for (uint32_t i = 0; i < image_count; ++i) 
            vkDestroyImageView(engine.device, swapchain_views[i], nullptr);
        std::allocator<VkImageView>().deallocate(swapchain_views, image_count);
        
        vkDestroySwapchainKHR(engine.device, old_swapchain, nullptr);
    }

    { // init swapchain views
        uint32_t image_count;
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr));
        std::vector<VkImage> images(image_count);
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, images.data()));
        
        swapchain_views = std::allocator<VkImageView>().allocate(image_count);

        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = colour_format;
        // swizzling -> not needed
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        // mip mapping -> not needed
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        
        for (uint32_t i = 0; i < images.size(); ++i) {
            info.image = images[i];
            VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &swapchain_views[i]));
        }
    }

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

    vkDeviceWaitIdle(engine.device);
}

auto Swapchain::create_shader_module(std::filesystem::path fp) -> VkShaderModule {
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
