#define VK_IMPLEMENTATION
#include "engine.h"
#include "renderer.h"

// basic properties
auto get_frame_count(SyncMode mode) -> uint32_t;
auto get_surface_data(VkSurfaceKHR surface) -> std::pair<VkFormat, VkColorSpaceKHR>;
auto get_present_mode(VkSurfaceKHR surface, VsyncMode vsync_mode) -> VkPresentModeKHR;
auto get_sample_count(AntiAlias anti_alias) -> VkSampleCountFlagBits;
auto get_memory_index(uint32_t type_bits, VkMemoryPropertyFlags flags) -> uint32_t;

// init swapchain
auto init_swapchain(VkSurfaceKHR surface, uint32_t res_x, uint32_t res_y, VkFormat format, VkColorSpaceKHR colour_space,
    VkPresentModeKHR present_mode, uint32_t frame_count, VkSwapchainKHR old_swapchain=VK_NULL_HANDLE) -> VkSwapchainKHR;
auto get_swapchain_images(VkSwapchainKHR swapchain) -> std::vector<VkImage>;
auto init_swapchain_views(uint32_t width, uint32_t height, VkFormat format, std::span<VkImage> images) -> VkImageView*;

// init present pass
auto init_present_renderpass(VkFormat format, VkSampleCountFlagBits sample_count) -> VkRenderPass;
auto init_present_framebuffers(uint32_t width, uint32_t height, VkRenderPass renderpass, VkImageView render_view, std::span<VkImageView> swapchain_views) -> VkFramebuffer*;
auto init_present_attachment_image(uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits sample_count) -> VkImage;
auto init_present_attachment_memory(VkImage image) -> VkDeviceMemory;
auto init_present_attachment_view(VkImage image, VkFormat format) -> VkImageView;



Renderer::Renderer(const Window& window, uint32_t res_x, uint32_t res_y, AntiAlias alias_mode, VsyncMode vsync_mode, SyncMode sync_mode) 
 : window(window), resolution({ res_x, res_y }), anti_alias(alias_mode), vsync_mode(vsync_mode), frame_count(get_frame_count(sync_mode)), frame_index(0) {
    // get window surface
    VK_ASSERT(glfwCreateWindowSurface(engine.instance, window.window, nullptr, &surface));

    auto [format, colour_space] = get_surface_data(surface);
    auto present_mode = get_present_mode(surface, vsync_mode);
    auto sample_count = get_sample_count(anti_alias);
    auto [width, height] = window.get_size();
    
    // init swapchain
    swapchain = init_swapchain(surface, width, height, format, colour_space, present_mode, frame_count, VK_NULL_HANDLE);
    auto images = get_swapchain_images(swapchain);
    swapchain_views = init_swapchain_views(width, height, format, images);

    { // init frames
        VkCommandBufferAllocateInfo graphics_cmd_pool_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, engine.graphics.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
        VkCommandBufferAllocateInfo compute_cmd_pool_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, engine.compute.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1 };
        VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
        VkFenceCreateInfo in_flight_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };

        for (uint32_t i = 0; i < frame_count; ++i) {
            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &graphics_cmd_pool_info, &graphics_cmd_buffer[i]));
            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &compute_cmd_pool_info, &compute_cmd_buffer[i]));
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore_info, nullptr, &image_available[i]));
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore_info, nullptr, &render_finished[i]));
            VK_ASSERT(vkCreateFence(engine.device, &in_flight_info, nullptr, &in_flight[i]));
        }       
    }

    // init descriptor layouts
    {
        VkDescriptorSetLayoutBinding bindings[3];
        bindings[0].binding = 0; // lights
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        bindings[1].binding = 1; // materials
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        bindings[2].binding = 2; // materials
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

        VK_ASSERT(vkCreateDescriptorSetLayout(engine.device, &info, nullptr, &layout));
    }

    // init present pass
    present_pass.attachment.image = init_present_attachment_image(width, height, format, sample_count);
    present_pass.attachment.memory = init_present_attachment_memory(present_pass.attachment.image);
    present_pass.attachment.view = init_present_attachment_view(present_pass.attachment.image, format);
    
    present_pass.renderpass = init_present_renderpass(format, sample_count);
    present_pass.framebuffers = init_present_framebuffers(width, height, present_pass.renderpass, present_pass.attachment.view, { swapchain_views, images.size() });
}

Renderer::~Renderer() {
    // destroy present pass attachment
    vkDestroyRenderPass(engine.device, present_pass.renderpass, nullptr);
    vkDestroyImageView(engine.device, present_pass.attachment.view, nullptr);
    vkDestroyImage(engine.device, present_pass.attachment.image, nullptr);
    vkFreeMemory(engine.device, present_pass.attachment.memory, nullptr);

    for (uint32_t i = 0; i < frame_count; ++i) {
        vkFreeCommandBuffers(engine.device, engine.graphics.pool, 1, &graphics_cmd_buffer[i]);
        vkFreeCommandBuffers(engine.device, engine.compute.pool, 1,  &compute_cmd_buffer[i]);
        vkDestroySemaphore(engine.device, image_available[i], nullptr);
        vkDestroySemaphore(engine.device, render_finished[i], nullptr);
        vkDestroyFence(engine.device, in_flight[i], nullptr);
    }

    uint32_t image_count;
    vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr);

    for (uint32_t i = 0; i < image_count; ++i) vkDestroyImageView(engine.device, swapchain_views[i], nullptr);
    std::allocator<VkImageView>().deallocate(swapchain_views, image_count);

    for (uint32_t i = 0; i < image_count; ++i) vkDestroyFramebuffer(engine.device, present_pass.framebuffers[i], nullptr);
    std::allocator<VkFramebuffer>().deallocate(present_pass.framebuffers, image_count);

    vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
    
    vkDestroySurfaceKHR(engine.instance, surface, nullptr);
}

void Renderer::set_resolution(uint32_t res_x, uint32_t res_y) {
    // TODO:    
}

std::pair<uint32_t, uint32_t> Renderer::get_resolution() const {
    return { resolution.x, resolution.y };
}

std::vector<std::pair<uint32_t, uint32_t>> Renderer::enum_resolutions() const {
    return { { 640, 480 }, { 800, 600 }, { 1024, 768 }, { 1280, 960 }, { 1400, 1050 } };
}

void Renderer::set_vsync_mode(VsyncMode mode) { 
    // TODO:
}

VsyncMode Renderer::get_vsync_mode() const {
    return vsync_mode;
}

std::vector<VsyncMode> Renderer::enum_vsync_modes() const {
    std::vector<VsyncMode> vsync_modes;

    vsync_modes.push_back(VsyncMode::ON); // guaranteed to be supported VK_PRESENT_MODE_FIFO_KHR

    uint32_t count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> supported(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data());

    if (std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != supported.end() || 
        std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end())
    {
        vsync_modes.push_back(VsyncMode::OFF);
    }

    return vsync_modes;   
}

void Renderer::set_sync_mode(SyncMode mode) {
    // TODO:
}

SyncMode Renderer::get_sync_mode() const {
    switch (frame_count) {
        case 2: return SyncMode::DOUBLE;
        case 3: return SyncMode::TRIPLE;
    }
}

std::vector<SyncMode> Renderer::enum_sync_modes() const {
    std::vector<SyncMode> sync_modes;
    sync_modes.push_back(SyncMode::DOUBLE);
    
    uint32_t count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> supported(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data());

    
    return sync_modes;
}


void Renderer::set_anti_alias(AntiAlias mode) {
    // TODO: recreate present pipeline
}

AntiAlias Renderer::get_anti_alias() const {
    return anti_alias;
}

std::vector<AntiAlias> Renderer::enum_ant_alias() const {
    std::vector<AntiAlias> anti_alias_modes { AntiAlias::NONE, AntiAlias::FXAA_2, AntiAlias::FXAA_4, AntiAlias::FXAA_8, AntiAlias::FXAA_16 };
    
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

void Renderer::draw() {
    const uint64_t timeout = 1000000000;

    VK_ASSERT(vkWaitForFences(engine.device, 1, &in_flight[frame_index], true, timeout));
    vkResetFences(engine.device, 1, &in_flight[frame_index]);
    
    uint32_t image_index;
    { // get next swapchain image
        VkResult res = vkAcquireNextImageKHR(engine.device, swapchain, timeout, image_available[frame_index], nullptr, &image_index);
    
        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            return;
        } else if (res != VK_SUBOPTIMAL_KHR) {
            VK_ASSERT(res);
        }
    }

    // TODO: update uniform buffers

    VkCommandBuffer cmd_buffer = graphics_cmd_buffer[frame_index];
    vkResetCommandBuffer(cmd_buffer, 0);
    
    { // begin cmd_buffer
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer, &info));
    }

    { // begin present pass
        VkClearValue clear_value[1];
        clear_value[0].color = {{ 0, 0, 0, 0 }};
        clear_value[0].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.pNext = nullptr;
        info.renderPass = present_pass.renderpass;
        info.framebuffer = present_pass.framebuffers[image_index];
        info.renderArea.offset = { 0, 0 };
        info.renderArea.extent = { resolution.x, resolution.y };
        info.clearValueCount = 1;
        info.pClearValues = clear_value;

        vkCmdBeginRenderPass(cmd_buffer, &info, VK_SUBPASS_CONTENTS_INLINE);

        // bind pipeline

        // set dynamics

        // bind vertex buffers

        // bind index buffer

        // bind descriptor sets

        // vkCmdDrawIndexed

        vkCmdEndRenderPass(cmd_buffer);
    }

    VK_ASSERT(vkEndCommandBuffer(cmd_buffer));


    { // submit cmd_buffer
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.pNext = nullptr;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_available[frame_index];
        info.pWaitDstStageMask = wait_stages;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &cmd_buffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_finished[frame_index];

        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, in_flight[frame_index]));
    }

    { // present to screen
        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;;
        info.pNext = nullptr;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &render_finished[frame_index];
        info.swapchainCount = 1;
        info.pSwapchains = &swapchain;
        info.pImageIndices = &image_index;
        info.pResults = nullptr;
        
        VkResult res = vkQueuePresentKHR(engine.present.queue, &info);

        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            // TODO: recreate swapchain
        } else if (res != VK_SUBOPTIMAL_KHR) {
            VK_ASSERT(res);
        }
    }

    frame_index = (frame_index + 1) % frame_count; // get next frame data set
}

auto get_frame_count(SyncMode mode) -> uint32_t {
    return static_cast<uint32_t>(mode);
}

auto get_surface_data(VkSurfaceKHR surface) -> std::pair<VkFormat, VkColorSpaceKHR> {
    VkFormat format;
    VkColorSpaceKHR colour_space;
    { // get format
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, nullptr);
        std::vector<VkSurfaceFormatKHR> supported(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, supported.data());

        for (VkSurfaceFormatKHR& surface_format : supported) {
            switch (surface_format.format) {
                case(VK_FORMAT_R8G8B8A8_SRGB): break;
                case(VK_FORMAT_B8G8R8A8_SRGB): break;
                case(VK_FORMAT_R8G8B8A8_UNORM): break;
                case(VK_FORMAT_B8G8R8A8_UNORM): break;
                default: continue;
            }
            format = surface_format.format;
            colour_space = surface_format.colorSpace;
        }
    }

    return { format, colour_space };
}

auto get_present_mode(VkSurfaceKHR surface, VsyncMode vsync_mode) -> VkPresentModeKHR {
    /*  https://registry.khronos.org/vulkan/specs/latest/man/html/VkPresentModeKHR.html
    vsync off
    VK_PRESENT_MODE_IMMEDIATE_KHR - standard vsync off
    VK_PRESENT_MODE_FIFO_RELAXED_KHR - will skip a swapchain image if a new image available
    vsync on
    VK_PRESENT_MODE_FIFO_KHR - standard vsync on
    VK_PRESENT_MODE_MAILBOX_KHR - will skip a swapchain image if a new image available
    vsync adaptive
    VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR - dynamic fps, refreshes on new image available, allows no refresh
    VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR - fixed fps, refreshes continuously
    */

    uint32_t count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> supported(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data());
        
    bool frame_skip_support;

    switch (vsync_mode) {
    case VsyncMode::OFF:
        frame_skip_support = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end(); 
        return frame_skip_support ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    case VsyncMode::ON:
        frame_skip_support = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_MAILBOX_KHR) != supported.end();
        return frame_skip_support ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
    // ADAPTIVE ???
    }
}

auto get_sample_count(AntiAlias anti_alias) -> VkSampleCountFlagBits {
    switch (anti_alias) {
        case AntiAlias::NONE:    return VK_SAMPLE_COUNT_1_BIT;
        // FXAA is implemented in a post processor shader 
        case AntiAlias::FXAA_2:  return VK_SAMPLE_COUNT_1_BIT;
        case AntiAlias::FXAA_4:  return VK_SAMPLE_COUNT_1_BIT;
        case AntiAlias::FXAA_8:  return VK_SAMPLE_COUNT_1_BIT;
        case AntiAlias::FXAA_16: return VK_SAMPLE_COUNT_1_BIT;
        // MSAA is implemented in a the renderer sampling
        case AntiAlias::MSAA_2:  return VK_SAMPLE_COUNT_2_BIT;
        case AntiAlias::MSAA_4:  return VK_SAMPLE_COUNT_4_BIT;
        case AntiAlias::MSAA_8:  return VK_SAMPLE_COUNT_8_BIT;
        case AntiAlias::MSAA_16: return VK_SAMPLE_COUNT_16_BIT;
    }
}

auto get_memory_index(uint32_t type_bits, VkMemoryPropertyFlags flags) -> uint32_t {
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(engine.gpu, &properties);

    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((type_bits & (1 << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    assert(false);
}

auto init_swapchain(VkSurfaceKHR surface, uint32_t width, uint32_t height, VkFormat format, VkColorSpaceKHR colour_space, 
    VkPresentModeKHR present_mode, uint32_t frame_count, VkSwapchainKHR old_swapchain) -> VkSwapchainKHR {
    VkSwapchainKHR swapchain;

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.pNext = nullptr;
    info.flags = 0;
    info.surface = surface;
    info.minImageCount = frame_count;
    info.imageFormat = format;
    info.imageColorSpace = colour_space;
    info.imageExtent = { width, height };
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
    return swapchain;
}

auto init_present_renderpass(VkFormat format, VkSampleCountFlagBits msaa_sample) -> VkRenderPass {
    VkRenderPass renderpass;
    
    VkAttachmentDescription attachments[2];
    // colour attachment
    attachments[0].flags = 0;
    attachments[0].format = format;
    attachments[0].samples = msaa_sample;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // resolve attachment
    attachments[1].format = format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[1].flags = 0;

    VkAttachmentReference col_refs[1];
    col_refs[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    
    VkAttachmentReference res_refs[1];
    res_refs[0] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpasses[1];
    subpasses[0].flags = 0;
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].inputAttachmentCount = 0;
    subpasses[0].pInputAttachments = nullptr;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = col_refs;
    subpasses[0].pResolveAttachments = msaa_sample == VK_SAMPLE_COUNT_1_BIT ? nullptr : res_refs;
    subpasses[0].pDepthStencilAttachment = nullptr;
    subpasses[0].preserveAttachmentCount = 0;
    subpasses[0].pPreserveAttachments = nullptr;
    
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
    info.attachmentCount = 2;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = subpasses;
    info.dependencyCount = 1;
    info.pDependencies = dependencies;

    VK_ASSERT(vkCreateRenderPass(engine.device, &info, nullptr, &renderpass));

    return renderpass;
}

auto get_swapchain_images(VkSwapchainKHR swapchain) -> std::vector<VkImage> {
    uint32_t image_count;
    VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr));
    std::vector<VkImage> images(image_count);
    VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, images.data()))
    return images;
}

auto init_swapchain_views(uint32_t width, uint32_t height, VkFormat format, std::span<VkImage> images) -> VkImageView* {
    VkImageView* views = std::allocator<VkImageView>().allocate(images.size());
    
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
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
        VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &views[i]));
    }
    return views;
}

auto init_present_framebuffers(uint32_t width, uint32_t height, VkRenderPass renderpass, VkImageView render_view, std::span<VkImageView> swapchain_views) -> VkFramebuffer* {
    VkFramebuffer* framebuffers = std::allocator<VkFramebuffer>().allocate(swapchain_views.size());

    VkImageView attachments[2] { render_view, nullptr };

    VkFramebufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.renderPass = renderpass;
    info.attachmentCount = 2;
    info.pAttachments = attachments;
    info.width = width;
    info.height = height;
    info.layers = 1;

    for (uint32_t i = 0; i < swapchain_views.size(); ++i) {
        attachments[1] = swapchain_views[i];
        VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &framebuffers[i]));
    }

    return framebuffers;
}

auto init_present_attachment_image(uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits sample_count) -> VkImage {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.extent.width = width;
    info.extent.height = height;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.format = format;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.samples = sample_count;
    info.sharingMode = engine.graphics.family == engine.present.family ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

    VkImage image;
    VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &image));
    return image;
}

auto init_present_attachment_memory(VkImage image) -> VkDeviceMemory {
    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(engine.device, image, &requirements);

    VkMemoryAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.memoryTypeIndex = get_memory_index(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    info.allocationSize = requirements.size;

    VkDeviceMemory memory;
    VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &memory));

    VK_ASSERT(vkBindImageMemory(engine.device, image, memory, 0));

    return memory;
}

auto init_present_attachment_view(VkImage image, VkFormat format) -> VkImageView {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    
    VkImageView view;
    VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &view));
    return view;
}   