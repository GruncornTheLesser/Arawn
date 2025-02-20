#define VK_IMPLEMENTATION
#include "engine.h"
#include "renderer.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

auto init_pipeline_layout(VkDescriptorSetLayout set_layout) -> VkPipelineLayout; 
auto init_pipeline(VkRenderPass renderpass, VkPipelineLayout layout, VkShaderModule vert, VkShaderModule frag) -> VkPipeline;


Renderer::Renderer(const Window& window, AntiAlias alias_mode, VsyncMode vsync_mode, SyncMode sync_mode) 
 : window(window), anti_alias(alias_mode), vsync_mode(vsync_mode), frame_count(get_frame_count(sync_mode)), frame_index(0) 
{
    // get window surface
    VK_ASSERT(glfwCreateWindowSurface(engine.instance, window.window, nullptr, &surface));
    auto [colour_format, colour_space] = get_surface_data(surface);
    auto depth_format = get_depth_format();
    auto present_mode = get_present_mode(surface, vsync_mode);
    auto sample_count = get_sample_count(anti_alias);
    resolution = window.get_resolution();

    init_swapchain(colour_format, colour_space, present_mode, nullptr);
    init_present_framebuffer(depth_format, colour_format, sample_count);
    init_present_commands();
    //init_descriptors();

    auto vert = create_shader_module("res/import/shaders/def.vert.spv");
    auto frag = create_shader_module("res/import/shaders/def.frag.spv");
    
    present_pass.layout = init_pipeline_layout(set_layout);
    present_pass.pipeline = init_pipeline(present_pass.renderpass, present_pass.layout, vert, frag);
    
    vkDestroyShaderModule(engine.device, vert, nullptr);
    vkDestroyShaderModule(engine.device, frag, nullptr);
}

Renderer::~Renderer() {
    // destroy present pass attachment
    vkDeviceWaitIdle(engine.device);

    vkDestroyPipeline(engine.device, present_pass.pipeline, nullptr);
    vkDestroyPipelineLayout(engine.device, present_pass.layout, nullptr);
    
    //destroy_descriptors();
    destroy_present_commands();
    destroy_present_framebuffer();
    destroy_swapchain();

    vkDestroyDescriptorSetLayout(engine.device, set_layout, nullptr);
 
    vkDestroySurfaceKHR(engine.instance, surface, nullptr);
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

void Renderer::set_sync_mode(SyncMode mode) {
    // TODO:
}

SyncMode Renderer::get_sync_mode() const {
    if (frame_count == 2) return SyncMode::DOUBLE;
    else                  return SyncMode::TRIPLE;
}

std::vector<SyncMode> Renderer::enum_sync_modes() const {
    std::vector<SyncMode> sync_modes;
    sync_modes.push_back(SyncMode::DOUBLE);
    
    uint32_t count;
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
    std::vector<VkPresentModeKHR> supported(count);
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));
    
    return sync_modes;
}

void Renderer::set_anti_alias(AntiAlias mode) {
    // TODO: recreate present pipeline
}

AntiAlias Renderer::get_anti_alias() const {
    return anti_alias;
}

std::vector<AntiAlias> Renderer::enum_anti_alias() const {
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

void Renderer::draw() {
    const uint64_t timeout = 1000000000;
    
    uint32_t image_index;
    VkFence in_flight = present_pass.in_flight[frame_index];
    VkSemaphore image_available = present_pass.image_available[frame_index];
    
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
    
    VkCommandBuffer cmd_buffer = present_pass.cmd_buffers[frame_index];
    VkSemaphore finished = present_pass.finished[frame_index];
    
    VK_ASSERT(vkResetCommandBuffer(cmd_buffer, 0));
    { // record cmd buffer
        { // begin command buffer
            VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr };
            VK_ASSERT(vkBeginCommandBuffer(cmd_buffer, &info));
        }

        { // begin renderpass
            VkClearValue clear_value[2];
            clear_value[0].depthStencil = { 1.0f, 0 };
            clear_value[1].color = {{ 0.58f, 0, 0.84, 0 }};

            VkRenderPassBeginInfo info{};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.pNext = nullptr;
            info.renderPass = present_pass.renderpass;
            info.framebuffer = present_pass.framebuffers[image_index];
            info.renderArea.offset = { 0, 0 };
            info.renderArea.extent = { resolution.x, resolution.y };
            info.clearValueCount = 2;
            info.pClearValues = clear_value;
            
            vkCmdBeginRenderPass(cmd_buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
        }
        
        { // draw
            vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, present_pass.pipeline);
            { // set viewport
                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = (float) resolution.x;
                viewport.height = (float) resolution.y;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
            }
            { // set scissors 
                VkRect2D scissor{ };
                scissor.offset = {0, 0};
                scissor.extent = { resolution.x , resolution.y };
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
        info.pSignalSemaphores = &finished;

        VK_ASSERT(vkQueueSubmit(engine.graphics.queue, 1, &info, in_flight));
    }

    { // present to screen
        VkSemaphore wait[1] = { finished };

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

auto Renderer::get_frame_count(SyncMode mode)
 -> uint32_t {
    return static_cast<uint32_t>(mode);
}

auto Renderer::get_surface_data(VK_TYPE(VkSurfaceKHR) surface)
 -> std::pair<VK_TYPE(VkFormat), VK_TYPE(VkColorSpaceKHR)> {
    VkFormat format;
    VkColorSpaceKHR colour_space;
    
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
        format = surface_format.format;
        colour_space = surface_format.colorSpace;
    }

    return { format, colour_space };
}

auto Renderer::get_present_mode(VK_TYPE(VkSurfaceKHR) surface, VsyncMode vsync_mode)
 -> VK_TYPE(VkPresentModeKHR) {
    /*  https://registry.khronos.org/vulkan/specs/latest/man/html/VkPresentModeKHR.html
    vsync off
    VK_PRESENT_MODE_IMMEDIATE_KHR - standard vsync off
    VK_PRESENT_MODE_FIFO_RELAXED_KHR - vsync off, will skip a swapchain image if a new image available
    vsync on
    VK_PRESENT_MODE_FIFO_KHR - standard vsync on
    VK_PRESENT_MODE_MAILBOX_KHR - vsync on, will skip a swapchain image if a new image available
    */

    uint32_t count;
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
    std::vector<VkPresentModeKHR> supported(count);
    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));
        
    bool frame_skip_support;

    switch (vsync_mode) {
    case VsyncMode::OFF:
        frame_skip_support = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end(); 
        return frame_skip_support ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    case VsyncMode::ON:
        frame_skip_support = std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_MAILBOX_KHR) != supported.end();
        return frame_skip_support ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
    }
}

auto Renderer::get_sample_count(AntiAlias anti_alias)
 -> VK_TYPE(VkSampleCountFlagBits) {
    switch (anti_alias) {
        case AntiAlias::NONE:    return VK_SAMPLE_COUNT_1_BIT;
        // FXAA is implemented in a post processor shader 
        // case AntiAlias::FXAA_2:  return VK_SAMPLE_COUNT_1_BIT;
        // case AntiAlias::FXAA_4:  return VK_SAMPLE_COUNT_1_BIT;
        // case AntiAlias::FXAA_8:  return VK_SAMPLE_COUNT_1_BIT;
        // case AntiAlias::FXAA_16: return VK_SAMPLE_COUNT_1_BIT;
        // MSAA is implemented in a the renderer sampling
        case AntiAlias::MSAA_2:  return VK_SAMPLE_COUNT_2_BIT;
        case AntiAlias::MSAA_4:  return VK_SAMPLE_COUNT_4_BIT;
        case AntiAlias::MSAA_8:  return VK_SAMPLE_COUNT_8_BIT;
        case AntiAlias::MSAA_16: return VK_SAMPLE_COUNT_16_BIT;
    }
}

auto Renderer::get_depth_format() -> VK_TYPE(VkFormat) {
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(engine.gpu, VK_FORMAT_D32_SFLOAT, &properties);
    if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) 
        return VK_FORMAT_D32_SFLOAT;

    vkGetPhysicalDeviceFormatProperties(engine.gpu, VK_FORMAT_D32_SFLOAT_S8_UINT, &properties);
    if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) 
        return VK_FORMAT_D32_SFLOAT_S8_UINT;

    vkGetPhysicalDeviceFormatProperties(engine.gpu, VK_FORMAT_D24_UNORM_S8_UINT, &properties);
    if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) 
        return VK_FORMAT_D24_UNORM_S8_UINT;

    throw std::exception();
}

auto Renderer::get_memory_index(uint32_t type_bits, VK_TYPE(VkMemoryPropertyFlags) flags) -> uint32_t {
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(engine.gpu, &properties);

    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((type_bits & (1 << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    throw std::exception();
}

auto Renderer::create_shader_module(std::filesystem::path fp) -> VkShaderModule {
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

void Renderer::init_descriptors() { // init descriptor layouts
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

void Renderer::destroy_descriptors() {
    vkDestroyDescriptorSetLayout(engine.device, set_layout, nullptr);
}

void Renderer::init_present_commands() {
    VkCommandBufferAllocateInfo cmd_buffer_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, engine.graphics.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, frame_count };
    VK_ASSERT(vkAllocateCommandBuffers(engine.device, &cmd_buffer_info, present_pass.cmd_buffers));
    
    VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
    for (uint32_t i = 0; i < frame_count; ++i) {
        VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore_info, nullptr, &present_pass.finished[i]));
        VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore_info, nullptr, &present_pass.image_available[i]));
        VK_ASSERT(vkCreateFence(engine.device, &fence_info, nullptr, &present_pass.in_flight[i]));
    }
}

void Renderer::destroy_present_commands() {
    vkFreeCommandBuffers(engine.device, engine.graphics.pool, frame_count, present_pass.cmd_buffers);
    for (uint32_t i = 0; i < frame_count; ++i) {
        vkDestroySemaphore(engine.device, present_pass.finished[i], nullptr);
        vkDestroySemaphore(engine.device, present_pass.image_available[i], nullptr);
        vkDestroyFence(engine.device, present_pass.in_flight[i], nullptr);
    }
}

void Renderer::init_swapchain(VkFormat format, VkColorSpaceKHR colour_space, VkPresentModeKHR present_mode, VkSwapchainKHR old_swapchain) {
    { // init swapchain
        VkSwapchainCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.pNext = nullptr;
        info.flags = 0;
        info.surface = surface;
        info.minImageCount = frame_count;
        info.imageFormat = format;
        info.imageColorSpace = colour_space;
        info.imageExtent = { resolution.x, resolution.y };
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

    uint32_t image_count;
    VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr));
    std::vector<VkImage> images(image_count);
    VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, images.data()));
    
    swapchain_views = std::allocator<VkImageView>().allocate(image_count);
    {
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
            VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &swapchain_views[i]));
        }
    }
}

void Renderer::recreate_swapchain() {
    while (window.minimized()) { glfwWaitEvents(); }
    VK_ASSERT(vkDeviceWaitIdle(engine.device));

    destroy_present_framebuffer();
    destroy_swapchain();
    
    auto [colour_format, colour_space] = get_surface_data(surface);
    auto depth_format = get_depth_format();
    auto present_mode = get_present_mode(surface, vsync_mode);
    auto sample_count = get_sample_count(anti_alias);
    resolution = window.get_resolution();

    init_swapchain(colour_format, colour_space, present_mode, nullptr);
    init_present_framebuffer(depth_format, colour_format, sample_count);
}

void Renderer::destroy_swapchain() {
    uint32_t image_count;
    VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr));
    
    for (uint32_t i = 0; i < image_count; ++i) 
        vkDestroyImageView(engine.device, swapchain_views[i], nullptr);
    std::allocator<VkImageView>().deallocate(swapchain_views, image_count);
    
    vkDestroySwapchainKHR(engine.device, swapchain, nullptr);
}

void Renderer::init_present_framebuffer(VkFormat depth_format, VkFormat colour_format, VkSampleCountFlagBits sample_count) {
    // TODO: for a depth prepass, the depth attachment must be passed as an input attachment/not at all?
    bool msaa_enabled = sample_count != VK_SAMPLE_COUNT_1_BIT;
    bool depth_prepass_enabled = false;
    uint32_t attachment_count = msaa_enabled ? 3 : 2;
    
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
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT; // always 1
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

        VK_ASSERT(vkCreateRenderPass(engine.device, &info, nullptr, &present_pass.renderpass));
    }

    { // init depth attachment
        { // init image
            VkImageCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.extent = { resolution.x, resolution.y, 1 };
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.format = depth_format;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            info.samples = sample_count;
            info.sharingMode = engine.graphics.family == engine.present.family ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

            VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &present_pass.depth_attachment.image));
        }

        { // init memory
            VkMemoryRequirements requirements;
            vkGetImageMemoryRequirements(engine.device, present_pass.depth_attachment.image, &requirements);

            VkMemoryAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            info.pNext = nullptr;
            info.memoryTypeIndex = get_memory_index(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            info.allocationSize = requirements.size;

            VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &present_pass.depth_attachment.memory));
        }
        
        VK_ASSERT(vkBindImageMemory(engine.device, present_pass.depth_attachment.image, present_pass.depth_attachment.memory, 0));

        { // init view
            VkImageViewCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.image = present_pass.depth_attachment.image;
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

            VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &present_pass.depth_attachment.view));
        }
    }

    if (msaa_enabled) { // init colour attachment
        { // init image
            VkImageCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            info.imageType = VK_IMAGE_TYPE_2D;
            info.extent = { resolution.x, resolution.y, 1 };
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.format = colour_format;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            info.samples = sample_count;
            info.sharingMode = engine.graphics.family == engine.present.family ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;

            VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &present_pass.colour_attachment.image));
        }

        { // init memory
            VkMemoryRequirements requirements;
            vkGetImageMemoryRequirements(engine.device, present_pass.colour_attachment.image, &requirements);

            VkMemoryAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            info.pNext = nullptr;
            info.memoryTypeIndex = get_memory_index(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            info.allocationSize = requirements.size;

            VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &present_pass.colour_attachment.memory));
        }
        
        VK_ASSERT(vkBindImageMemory(engine.device, present_pass.colour_attachment.image, present_pass.colour_attachment.memory, 0));

        { // init view
            VkImageViewCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.image = present_pass.colour_attachment.image;
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

            VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &present_pass.colour_attachment.view));
        }
    } else {
        present_pass.colour_attachment.image = nullptr;
        present_pass.colour_attachment.memory = nullptr;
        present_pass.colour_attachment.view = nullptr;
    }

    { // init framebuffers
        uint32_t image_count;
        VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr));
        
        present_pass.framebuffers = std::allocator<VkFramebuffer>().allocate(image_count);
        
        VkImageView attachments[3] { present_pass.depth_attachment.view, nullptr, present_pass.colour_attachment.view };
        //                                                                  ^ swapchain image view

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;
        info.renderPass = present_pass.renderpass;
        info.attachmentCount = attachment_count;
        info.pAttachments = attachments;
        info.width = resolution.x;
        info.height = resolution.y;
        info.layers = 1;

        for (uint32_t i = 0; i < image_count; ++i) {
            attachments[1] = swapchain_views[i];
            VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &present_pass.framebuffers[i]));
        }
    }
}

void Renderer::destroy_present_framebuffer() {
    bool msaa_enabled = present_pass.colour_attachment.image != nullptr;
    
    uint32_t image_count;
    VK_ASSERT(vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr));

    for (uint32_t i = 0; i < image_count; ++i)
        vkDestroyFramebuffer(engine.device, present_pass.framebuffers[i], nullptr);
    std::allocator<VkFramebuffer>().deallocate(present_pass.framebuffers, image_count);

    vkDestroyImageView(engine.device, present_pass.depth_attachment.view, nullptr);
    vkFreeMemory(engine.device, present_pass.depth_attachment.memory, nullptr);
    vkDestroyImage(engine.device, present_pass.depth_attachment.image, nullptr);

    if (msaa_enabled) {
        vkDestroyImageView(engine.device, present_pass.colour_attachment.view, nullptr);
        vkFreeMemory(engine.device, present_pass.colour_attachment.memory, nullptr);
        vkDestroyImage(engine.device, present_pass.colour_attachment.image, nullptr);
    }
    
    vkDestroyRenderPass(engine.device, present_pass.renderpass, nullptr);
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

auto init_pipeline(VkRenderPass renderpass, VkPipelineLayout layout, VkShaderModule vert, VkShaderModule frag)
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
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // TODO: msaa sample count

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colour_attachment{};
    colour_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colour_attachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colour_blending{};
    colour_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colour_blending.logicOpEnable = VK_FALSE;
    colour_blending.logicOp = VK_LOGIC_OP_COPY;
    colour_blending.attachmentCount = 1;
    colour_blending.pAttachments = &colour_attachment;
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
