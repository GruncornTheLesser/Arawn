#define ARAWN_IMPLEMENTATION
#include <render/core/engine.h>
#include <display/window.h>
#include <algorithm>
#include <cassert>

namespace Arawn {
    void charCallback(GLFW_WINDOW window, unsigned int codepoint);
    void keyCallback(GLFW_WINDOW window, int key, int scancode, int action, int mods);
    void mouseMoveCallback(GLFW_WINDOW window, double xpos, double ypos);
    void mouseScrollCallback(GLFW_WINDOW window, double xoffset, double yoffset);
    void mouseButtonCallback(GLFW_WINDOW window, int button, int action, int mods);
    const GLFWvidmode* selectVideoMode(GLFWmonitor* monitor, int width, int height);
    VkFormat selectImageFormat(VkImageTiling tiling, VkFormatFeatureFlags features, const std::vector<VkFormat>& candidates);
}

Arawn::Window::Window(const Info& info) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    window = glfwCreateWindow(info.width, info.height, info.title, nullptr, nullptr);
    assert(window);

    VK_ASSERT(glfwCreateWindowSurface(engine.instance, window, nullptr, &surface));
    
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    //glfwSetCharCallback(window, charCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    uptime = std::chrono::high_resolution_clock::now();

    swapchain = nullptr;
    recreate(info);
}

void Arawn::Window::recreate(const Info& info) {
    
    // update window parameters
    if (!std::strcmp(state.title, info.title) || state.low_latency != info.low_latency || state.triple_buffered != info.triple_buffered || state.vsync != info.vsync || state.mode != info.mode) {
        glfwSetWindowTitle(window, info.title);
        
        switch (info.mode) {
            case DisplayMode::EXCLUSIVE: {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                const GLFWvidmode* vid_mode = selectVideoMode(monitor, width, height);
                glfwSetWindowMonitor(window, monitor, 0, 0, vid_mode->width, vid_mode->height, GLFW_DONT_CARE);
                break;
            }
            case DisplayMode::FULLSCREEN: {
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                glfwSetWindowMonitor(window, nullptr, 0, 0, width, height, GLFW_DONT_CARE);
                
                glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
                glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
                break;
            }
            case DisplayMode::WINDOWED: {
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                glfwSetWindowMonitor(window, nullptr, 50, 50, width, height, GLFW_DONT_CARE);
                glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_FALSE);
                glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
                break;
            }
        }
    }
    
    // update swapchain
    uint32_t width, height, frame_count, image_count;
    VkSurfaceCapabilitiesKHR capabilities;
    { // query surface properties
        VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine.gpu, surface, &capabilities));
        
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            width  = std::clamp(capabilities.currentExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
            height = std::clamp(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }

        frame_count = info.triple_buffered ? 3 : 2;
        image_count = std::max<uint32_t>(frame_count, capabilities.minImageCount);
        
        if (capabilities.maxImageCount != 0) {
            image_count = std::min<uint32_t>(image_count, capabilities.maxImageCount);
        }
    }

    { // get present mode
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, nullptr));
        
        std::vector<VkPresentModeKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(engine.gpu, surface, &count, supported.data()));
        
        if (info.vsync) {
            bool low_latency = info.low_latency & std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_MAILBOX_KHR) != supported.end();
            mode = low_latency ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
        } 
        else {
            bool low_latency = info.low_latency & std::find(supported.begin(), supported.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR) != supported.end();
            mode = low_latency ? VK_PRESENT_MODE_FIFO_RELAXED_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    { // get surface colour format
        uint32_t count;
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, nullptr));
        
        std::vector<VkSurfaceFormatKHR> supported(count);
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(engine.gpu, surface, &count, supported.data()));

        for (VkSurfaceFormatKHR& surface_format : supported) {
            switch (surface_format.format)
            {
                case(VK_FORMAT_R8G8B8A8_SRGB): break;
                case(VK_FORMAT_B8G8R8A8_SRGB): break;
                case(VK_FORMAT_R8G8B8A8_UNORM): break;
                case(VK_FORMAT_B8G8R8A8_UNORM): break;
                default: continue;
            }
            format = surface_format.format;
            colour = surface_format.colorSpace;
        }
    }

    { // init swapchain
        // swapchain images are used by the graphics and present queues
        std::vector<uint32_t> families = { 
            engine.queue[PRESENT].family, 
            engine.queue[GRAPHICS].family 
        };
        
        std::sort(families.begin(), families.end());
        families.erase(std::unique(families.begin(), families.end()), families.end());

        VkSwapchainCreateInfoKHR info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, 
            .pNext = nullptr,
            .surface = surface,
            .minImageCount = image_count, 
            .imageFormat = format,
            .imageColorSpace = colour, 
            .imageExtent = { width, height }, 
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
            .pQueueFamilyIndices = families.data(), 
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = mode,
            .clipped = VK_TRUE, // skip draw to obscured pixels
            .oldSwapchain = swapchain
        };

        VK_ASSERT(vkCreateSwapchainKHR(engine.device, &info, nullptr, &swapchain));
    }

    { // get swapchain image views
        // destroy old swap image views
        for (auto& swap : swaps) {
            vkDestroyImageView(engine.device, swap.view, nullptr);
        }

        // get new swapchain images
        vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, nullptr);
        
        std::vector<VkImage> images(image_count);
        vkGetSwapchainImagesKHR(engine.device, swapchain, &image_count, images.data());

        // assign image
        swaps.resize(image_count);
        for (uint32_t i = 0; i < image_count; ++i) {
            swaps[i].image = images[i];
        }

        // create views
        for (uint32_t i = 0; i < image_count; ++i) {
            VkImageViewCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = swaps[i].image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = format,
                .components = { },
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };
            vkCreateImageView(engine.device, &info, nullptr, &swaps[i].view);
        }
    }
    
    { // recreate frame data
        // modify existing frames
        if (state.width != info.width || state.height != info.height) {
            for (uint32_t i = 0; i < frames.size(); ++i) {
                frames[i].resize(*this, info.width, info.height);
            }
        }
        
        // create/destroy additional frames
        if (frame_count != frames.size()) {
            frames.reserve(frame_count);
            frames.erase(frames.begin() + frame_count, frames.end());
            for (uint32_t i = frames.size(); i < frame_count; ++i) {
                frames.emplace_back(*this, info);
            }
        }
    }

    state = info;
}

Arawn::Window::~Window() {
    if (window != nullptr)
    {
        vkDestroyDescriptorSetLayout(engine.device, pass.mesh_culling.descriptor_layout, nullptr);
        vkDestroyDescriptorSetLayout(engine.device, pass.depth.descriptor_layout, nullptr);
        vkDestroyDescriptorSetLayout(engine.device, pass.light_culling.descriptor_layout, nullptr);
        vkDestroyDescriptorSetLayout(engine.device, pass.forward.descriptor_layout, nullptr);
        // vkDestroyDescriptorSetLayout(engine.device, pass.queue[PRESENT].descriptor_layout, nullptr);

        vmaDestroyVirtualBlock(resource.mesh.cache.block);
        vmaDestroyBuffer(engine.allocator, resource.mesh.cache.buffer, resource.mesh.cache.memory);
        
        frames.clear();
        
        for (auto& swap : swaps) {
            vkDestroyImageView(engine.device, swap.view, nullptr);
        }

        vkDestroySwapchainKHR(engine.device, swapchain, nullptr);

        vkDestroySurfaceKHR(engine.instance, surface, nullptr);

        glfwDestroyWindow(window);

        window = nullptr;
    }
}

Arawn::Window::Window(Window&& other) : window(other.window), surface(other.surface), swapchain(other.swapchain), swaps(std::move(other.swaps)) {
    if (this == &other) {
        return;
    }

    glfwSetWindowUserPointer(window, this);
    other.window = nullptr;
}

Arawn::Window& Arawn::Window::operator=(Window&& other) {
    if (this == &other) {
        return *this;
    }
        
    if (window != nullptr) {
        for (auto& swap : swaps) {
            vkDestroyImageView(engine.device, swap.view, nullptr);
        }

        vkDestroySwapchainKHR(engine.device, swapchain, nullptr);

        vkDestroySurfaceKHR(engine.instance, surface, nullptr);

        glfwDestroyWindow(window);
    }

    
    window = other.window;
    surface = other.surface;
    swapchain = other.swapchain;
    swaps = std::move(other.swaps);

    glfwSetWindowUserPointer(window, this);
    other.window = nullptr;

    return *this;
}

auto Arawn::Window::closed() const -> bool {
    return glfwWindowShouldClose(window);
}

auto Arawn::Window::minimized() const -> bool {
    int x, y;
    glfwGetWindowSize(window, &x, &y);
    return x == 0 || y == 0;
}
/*
auto Window::mouse_position() const -> glm::vec2 {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return { x, y };
}
*/

void Arawn::Window::poll() {
    glfwPollEvents();
}

void Arawn::Window::refresh() {
    time current_frame = std::chrono::high_resolution_clock::now();
    uptime = current_frame;
    
    auto& frame = frames[frame_index];

    const uint64_t timeout = 1000000000;
    VK_ASSERT(vkWaitForFences(engine.device, 1, &frame.finished, true, timeout));
    
    // acquire next swapchain image
    if (VkResult res = vkAcquireNextImageKHR(engine.device, swapchain, timeout, frame.acquired, nullptr, &image_index); res == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate(state); // recreate if out of date
        return;
    } else if (res != VK_SUBOPTIMAL_KHR) {
        VK_ASSERT(res);
    }
    
    VK_ASSERT(vkResetFences(engine.device, 1, &frame.finished));

    // Transfer-Pass -> Mesh-Pass -> Z-Depth-Pass -> Light-Pass -> Forward-Pass -> Present Pass

    
    { // transfer submit
        VkSemaphore waits[]{ 
            frame.pass.transfer.finished
        };
        VkPipelineStageFlags stage[]{
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        };
    
        VkSubmitInfo transfer_submit[1]{
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, 
                .pNext = nullptr, 
                .waitSemaphoreCount = 1, 
                .pWaitSemaphores = waits,
                .pWaitDstStageMask = stage,
                .commandBufferCount = 1,
                .pCommandBuffers = &frame.pass.transfer.cmd, 
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &frame.pass.transfer.finished,
            }
        };
        VK_ASSERT(vkQueueSubmit(engine.queue[TRANSFER].queue, 1, transfer_submit, nullptr));
    }
        
    // compute submit
    {
        VkSemaphore waits1[]{
            frame.pass.transfer.finished
        };
        VkPipelineStageFlags stages1[]{
            VK_PIPELINE_STAGE_TRANSFER_BIT
        };
        VkSemaphore waits2[]{
            frame.pass.depth.finished
        };
        VkPipelineStageFlags stages2[]{
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        };

        VkSubmitInfo compute_submit[]{
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, 
                .pNext = nullptr, 
                .waitSemaphoreCount = 0, 
                .pWaitSemaphores = waits1,
                .pWaitDstStageMask = stages1,
                .commandBufferCount = 1,
                .pCommandBuffers = &frame.pass.mesh_culling.cmd, 
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &frame.pass.mesh_culling.finished,
            },

            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, 
                .pNext = nullptr, 
                .waitSemaphoreCount = 1, 
                .pWaitSemaphores = waits2,
                .pWaitDstStageMask = stages2,
                .commandBufferCount = 1,
                .pCommandBuffers = &frame.pass.light_culling.cmd, 
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &frame.pass.light_culling.finished,
            }
        };

        VK_ASSERT(vkQueueSubmit(engine.queue[COMPUTE].queue, 2, compute_submit, nullptr));
    }

    // graphics submit
    {
        VkSemaphore waits1[]{
            frame.pass.mesh_culling.finished
        };
        VkPipelineStageFlags stages1[]{
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        };
        VkSemaphore waits2[]{
            frame.pass.depth.finished
        };
        VkPipelineStageFlags stages2[]{
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        };


        VkSubmitInfo graphics_submit[]{
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, 
                .pNext = nullptr, 
                .waitSemaphoreCount = 1, 
                .pWaitSemaphores = waits1,
                .pWaitDstStageMask = stages1,
                .commandBufferCount = 1,
                .pCommandBuffers = &frame.pass.depth.cmd, 
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &frame.pass.depth.finished,
            },
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, 
                .pNext = nullptr, 
                .waitSemaphoreCount = 1, 
                .pWaitSemaphores = waits2,
                .pWaitDstStageMask = stages2,
                .commandBufferCount = 1,
                .pCommandBuffers = &frame.pass.forward.cmd, 
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &frame.pass.forward.finished,
            }
        };

        VK_ASSERT(vkQueueSubmit(engine.queue[GRAPHICS].queue, 2, graphics_submit, nullptr));
    }
    
    // present submit
    {
        VkSemaphore waits[]{
            frame.pass.forward.finished
        };
        VkPipelineStageFlags stages[]{
            VK_PIPELINE_STAGE_TRANSFER_BIT
        };

        VkSubmitInfo present_submit[1]{
            {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, 
                .pNext = nullptr, 
                .waitSemaphoreCount = 1, 
                .pWaitSemaphores = waits,
                .pWaitDstStageMask = stages,
                .commandBufferCount = 1, 
                .pCommandBuffers = &frame.pass.present.cmd, 
                .signalSemaphoreCount = 1, 
                .pSignalSemaphores = &frame.pass.present.finished,
            }
        };

        VK_ASSERT(vkQueueSubmit(engine.queue[PRESENT].queue, 1, present_submit, nullptr));
    }
    {
        VkSemaphore waits[]{
            frame.pass.present.finished
        };

        VkPresentInfoKHR present{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, 
            .pNext = nullptr, 
            .waitSemaphoreCount = 1, 
            .pWaitSemaphores = waits,
            .swapchainCount = 1, 
            .pSwapchains = &swapchain, 
            .pImageIndices = &image_index, 
            .pResults = nullptr

        };
        if (VkResult res = vkQueuePresentKHR(engine.queue[PRESENT].queue, &present); res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            return;
        } else {
            VK_ASSERT(res);
        }
    }

    frame_index = (frame_index + 1) % static_cast<uint32_t>(frames.size()); // increment frame
}

Arawn::Window::Frame::Frame(const Window& window, const Info& info) {
    
    { // camera resources
        { // camera uniform buffer
            std::vector<uint32_t> families = { 
                engine.queue[GRAPHICS].family,
                engine.queue[COMPUTE].family,
                engine.queue[TRANSFER].family
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
    
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = sizeof(Camera),
                .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .priority = 1.0f,
            };
    
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.camera.uniform.buffer, &resource.camera.uniform.memory, nullptr));
        }
    
        { // camera staging buffer
            std::vector<uint32_t> families = { 
                engine.queue[TRANSFER].family
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
    
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = sizeof(Camera),
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                .priority = 1.0f,
            };
    
            VmaAllocationInfo info;
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.camera.staging.buffer, &resource.camera.staging.memory, &info));
            
            resource.camera.staging.mapped = static_cast<Camera*>(info.pMappedData);
    
            // TODO: there is a feature called integrated bar memory which would negate the need for separate staging buffers
        }
    }

    { // light resources
        { // light source buffer
            std::vector<uint32_t> families = { 
                engine.queue[COMPUTE].family,
                engine.queue[GRAPHICS].family,
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
    
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = sizeof(Light) * window.max_lights,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .priority = 1.0f,
            };
    
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.light.source.buffer, &resource.light.source.memory, nullptr));
        }
    }

    { // mesh resources
        { // mesh manifest buffer
            std::vector<uint32_t> families = { 
                engine.queue[TRANSFER].family,
                engine.queue[COMPUTE].family,
                engine.queue[GRAPHICS].family,
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
    
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = sizeof(Transform) * max_instance,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .priority = 1.0f,
            };
    
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.mesh.manifest.buffer, &resource.mesh.manifest.memory, nullptr));
        }

        { // mesh staging buffer
            std::vector<uint32_t> families = { 
                engine.queue[TRANSFER].family
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
    
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = sizeof(MeshInstance) * max_instance,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                .priority = 1.0f,
            };
    
            VmaAllocationInfo info;
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.mesh.staging.buffer, &resource.mesh.staging.memory, &info));
            
            resource.mesh.staging.mapped = static_cast<MeshInstance*>(info.pMappedData);
        }

        { // mesh indirect buffer
            std::vector<uint32_t> families = { 
                engine.queue[COMPUTE].family,
                engine.queue[GRAPHICS].family,
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
            
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = sizeof(VkDrawMeshTasksIndirectCommandEXT) budget / (meshlet_size * 3 * sizeof(uint32_t)),
                .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .priority = 1.0f,
            };
    
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.mesh.indirect.buffer, &resource.mesh.indirect.memory, nullptr));
        }
    
        { // mesh count buffer
            std::vector<uint32_t> families = { 
                engine.queue[COMPUTE].family,
                engine.queue[GRAPHICS].family,
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
            
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = sizeof(uint32_t) * max_draws,
                .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .priority = 1.0f,
            };
    
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.mesh.count.buffer, &resource.mesh.count.memory, nullptr));
        }
    
        { // mesh transform buffer
            std::vector<uint32_t> families = { 
                engine.queue[COMPUTE].family,
                engine.queue[GRAPHICS].family,
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
    
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = sizeof(Transform) * max_instance,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .priority = 1.0f,
            };
    
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.mesh.transform.buffer, &resource.mesh.transform.memory, nullptr));
        }
    }
    
    { // pass sync objects
        { // transfer pass
            VkSemaphoreCreateInfo semaphore{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
            };
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore, nullptr, &pass.queue[TRANSFER].finished));

            VkCommandBufferAllocateInfo cmd{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = engine.queue[COMPUTE].pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &cmd, &pass.queue[TRANSFER].cmd));
        }

        { // mesh culling pass
            struct VkSemaphoreCreateInfo semaphore{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
            };
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore, nullptr, &pass.mesh_culling.finished));
            
            struct VkCommandBufferAllocateInfo cmd{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = engine.queue[COMPUTE].pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &cmd, &pass.mesh_culling.cmd));

            VkDescriptorSetAllocateInfo descriptor{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                // .pool = nullptr, /*TODO pool*/
                .descriptorSetCount = 1,
                .pSetLayouts = &window.pass.mesh_culling.descriptor_layout,
            };
            VK_ASSERT(vkAllocateDescriptorSets(engine.device, &descriptor, pass.mesh_culling.descriptor));

            
        }

        { // depth pass 
            struct VkSemaphoreCreateInfo semaphore{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
            };
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore, nullptr, &pass.depth.finished));
            
            struct VkCommandBufferAllocateInfo cmd{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = engine.queue[COMPUTE].pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &cmd, &pass.depth.cmd));

            VkDescriptorSetAllocateInfo descriptor{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                // .pool = nullptr, /*TODO pool*/
                .descriptorSetCount = 1,
                .pSetLayouts = &window.pass.depth.descriptor_layout,
            };
            VK_ASSERT(vkAllocateDescriptorSets(engine.device, &descriptor, pass.depth.descriptor));
        }

        { // light culling pass
            struct VkSemaphoreCreateInfo semaphore{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
            };
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore, nullptr, &pass.light_culling.finished));
            
            struct VkCommandBufferAllocateInfo cmd{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = engine.queue[COMPUTE].pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &cmd, &pass.light_culling.cmd));

            VkDescriptorSetAllocateInfo descriptor{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                // .pool = nullptr, /*TODO pool*/
                .descriptorSetCount = 1,
                .pSetLayouts = &window.pass.light_culling.descriptor_layout,
            };
            VK_ASSERT(vkAllocateDescriptorSets(engine.device, &descriptor, pass.light_culling.descriptor));
        }

        { // forward pass 
            struct VkSemaphoreCreateInfo semaphore{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
            };
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore, nullptr, &pass.forward.finished));
            
            struct VkCommandBufferAllocateInfo cmd{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = engine.queue[COMPUTE].pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };
            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &cmd, &pass.forward.cmd));

            VkDescriptorSetAllocateInfo descriptor{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                // .pool = nullptr, /*TODO pool*/
                .descriptorSetCount = 1,
                .pSetLayouts = &window.pass.forward.descriptor_layout,
            };
            VK_ASSERT(vkAllocateDescriptorSets(engine.device, &descriptor, pass.forward.descriptor));
        }

        { // frame 
            VkFenceCreateInfo fence{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };
    
            VK_ASSERT(vkCreateFence(engine.device, &fence, nullptr, &finished));
    
            struct VkSemaphoreCreateInfo semaphore{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
            };
            VK_ASSERT(vkCreateSemaphore(engine.device, &semaphore, nullptr, &acquired));
        }
    } 
}

Arawn::Window::Frame::~Frame() {
    
    // camera resource
    vmaDestroyBuffer(engine.allocator, resource.camera.staging.buffer, resource.camera.staging.memory);
    vmaDestroyBuffer(engine.allocator, resource.camera.uniform.buffer, resource.camera.uniform.memory);

    // light resource
    vmaDestroyBuffer(engine.allocator, resource.light.cluster.buffer, resource.light.cluster.memory);
    vmaDestroyBuffer(engine.allocator, resource.light.source.buffer, resource.light.source.memory);
    vmaDestroyBuffer(engine.allocator, resource.light.culled.buffer, resource.light.culled.memory);

    // mesh resource
    vmaDestroyBuffer(engine.allocator, resource.mesh.manifest.buffer, resource.mesh.manifest.memory);
    vmaDestroyBuffer(engine.allocator, resource.mesh.staging.buffer, resource.mesh.staging.memory);
    vmaDestroyBuffer(engine.allocator, resource.mesh.indirect.buffer, resource.mesh.indirect.memory);
    vmaDestroyBuffer(engine.allocator, resource.mesh.count.buffer, resource.mesh.count.memory);
    vmaDestroyBuffer(engine.allocator, resource.mesh.transform.buffer, resource.mesh.transform.memory);

    // pass objects
    { // free graphics
        VkCommandBuffer cmds[] { pass.depth.cmd, pass.forward.cmd };
        vkFreeCommandBuffers(engine.device, engine.queue[GRAPHICS].pool, 2, cmds);
    }

    { // free compute
        VkCommandBuffer cmds[] { pass.light_culling.cmd, pass.mesh_culling.cmd };
        vkFreeCommandBuffers(engine.device, engine.queue[GRAPHICS].pool, 2, cmds);
    }

    { // free transfer
        VkCommandBuffer cmds[] { pass.queue[TRANSFER].cmd };
        vkFreeCommandBuffers(engine.device, engine.queue[GRAPHICS].pool, 1, cmds);
    }

    { // free present
        VkCommandBuffer cmds[] { pass.queue[PRESENT].cmd};
        vkFreeCommandBuffers(engine.device, engine.queue[GRAPHICS].pool, 1, cmds);
    }

    vkDestroySemaphore(engine.device, pass.queue[TRANSFER].finished, nullptr);
    vkDestroySemaphore(engine.device, pass.mesh_culling.finished, nullptr);
    vkDestroySemaphore(engine.device, pass.depth.finished, nullptr);
    vkDestroySemaphore(engine.device, pass.light_culling.finished, nullptr);
    vkDestroySemaphore(engine.device, pass.forward.finished, nullptr);
    vkDestroySemaphore(engine.device, pass.queue[PRESENT].finished, nullptr);
    vkDestroySemaphore(engine.device, acquired, nullptr);
    vkDestroyFence(engine.device, finished, nullptr);


    vkDestroyImageView(engine.device, resource.attachment.colour.view, nullptr);
    vmaDestroyImage(engine.allocator, resource.attachment.colour.image, resource.attachment.colour.memory);

    vkDestroyImageView(engine.device, resource.attachment.depth.view, nullptr);
    vmaDestroyImage(engine.allocator, resource.attachment.depth.image, resource.attachment.depth.memory);
}

void Arawn::Window::Frame::resize(const Window& window, uint32_t width, uint32_t height) {
    if (resource.light.cluster.buffer != nullptr) {
        // buffers dependent on cluster size
        vmaDestroyBuffer(engine.allocator, resource.light.cluster.buffer, resource.light.cluster.memory);
        vmaDestroyBuffer(engine.allocator, resource.light.culled.buffer, resource.light.culled.memory);
        vmaDestroyBuffer(engine.allocator, resource.mesh.vismask.buffer, resource.mesh.vismask.memory);

        vmaDestroyImage(engine.allocator, resource.attachment.colour.image, resource.attachment.colour.memory);
        vkDestroyImageView(engine.device, resource.attachment.colour.view, nullptr);

        vmaDestroyImage(engine.allocator, resource.attachment.depth.image, resource.attachment.depth.memory);
        vkDestroyImageView(engine.device, resource.attachment.depth.view, nullptr);
    }

    { // light resources
        { // light cluster buffer
            std::vector<uint32_t> families = { 
                engine.queue[COMPUTE].family,
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
    
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = sizeof(Cluster) * window.cluster.count.x * window.cluster.count.y,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .priority = 1.0f,
            };
    
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.light.cluster.buffer, &resource.light.cluster.memory, nullptr));
        }
    
        { // light culled buffer
            std::vector<uint32_t> families = { 
                engine.queue[COMPUTE].family,
                engine.queue[GRAPHICS].family,
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
    
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = sizeof(uint32_t) * window.cluster.count.x * window.cluster.count.y * window.cluster.max_lights,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .priority = 1.0f,
            };
    
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.light.culled.buffer, &resource.light.culled.memory, nullptr));
        }    
    }

    { // mesh resources
        { // mesh vismask buffer
            std::vector<uint32_t> families = { 
                engine.queue[COMPUTE].family,
                engine.queue[GRAPHICS].family,
            };
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
            
            VkBufferCreateInfo buffer{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = (window.max_meshlet + window.cluster.size - 1) / window.cluster.size * sizeof(uint32_t),
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
            };
    
            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .priority = 1.0f,
            };
    
            VK_ASSERT(vmaCreateBuffer(engine.allocator, &buffer, &alloc, &resource.mesh.vismask.buffer, &resource.mesh.vismask.memory, nullptr));
        } 

    }

    { // attachment resources
        { // colour attachment
            std::vector<uint32_t> families = { 
                engine.queue[GRAPHICS].family, 
                engine.queue[PRESENT].family
            }; 
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());

            VkFormat format = selectImageFormat(
                VK_IMAGE_TILING_OPTIMAL, 
                VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, 
                { 
                    VK_FORMAT_R8G8B8A8_SRGB, 
                    VK_FORMAT_B8G8R8A8_SRGB, 
                    VK_FORMAT_R8G8B8A8_UNORM, 
                    VK_FORMAT_B8G8R8A8_UNORM, 
                }
            );
            
            VkImageCreateInfo img{
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = format,
                .extent = { width, height, 1 },
                .mipLevels = 0,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .sharingMode = families.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
                .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };

            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .preferredFlags = 0,
                .memoryTypeBits = 0,
                .pool = nullptr,
                .pUserData = nullptr,
                .priority = 1.0f,
            };
            VK_ASSERT(vmaCreateImage(engine.allocator, &img, &alloc, &resource.attachment.colour.image, &resource.attachment.colour.memory, nullptr));
    
            VkImageViewCreateInfo view{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = resource.attachment.colour.image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = format,
                .components = {},
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };
            VK_ASSERT(vkCreateImageView(engine.device, &view, nullptr, &resource.attachment.colour.view));
        }
        
        { // depth attachment
            std::vector<uint32_t> families = { 
                engine.queue[GRAPHICS].family, 
                engine.queue[COMPUTE].family
            }; 
            
            std::sort(families.begin(), families.end());
            families.erase(std::unique(families.begin(), families.end()), families.end());
            
            VkFormat format = selectImageFormat(
                VK_IMAGE_TILING_OPTIMAL, 
                VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                { 
                    VK_FORMAT_D32_SFLOAT, 
                    VK_FORMAT_D32_SFLOAT_S8_UINT, 
                    VK_FORMAT_D24_UNORM_S8_UINT, 
                    VK_FORMAT_D16_UNORM, 
                    VK_FORMAT_D16_UNORM_S8_UINT, 
                }
            );
            
            VkImageCreateInfo img {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = format,
                .extent = { width, height, 1 },
                .mipLevels = 0,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                .sharingMode = families.size() == 1 ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
                .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                .pQueueFamilyIndices = families.data(),
                .initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            };

            VmaAllocationCreateInfo alloc{
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .preferredFlags = 0,
                .memoryTypeBits = 0,
                .pool = nullptr,
                .pUserData = nullptr,
                .priority = 1.0f,
            };
            VK_ASSERT(vmaCreateImage(engine.allocator, &img, &alloc, &resource.attachment.depth.image, &resource.attachment.depth.memory, nullptr));
    
            VkImageViewCreateInfo view{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = resource.attachment.depth.image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = format,
                .components = {},
                .subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 },
            };
            VK_ASSERT(vkCreateImageView(engine.device, &view, nullptr, &resource.attachment.depth.view));
        }
    }

    { // pass
        { // mesh cull record
            VkCommandBufferBeginInfo begin{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = 0,
                .pInheritanceInfo = nullptr,
            };
            vkBeginCommandBuffer(pass.mesh_culling.cmd, &begin);
            
            vkCmdFillBuffer(pass.mesh_culling.cmd, resource.mesh.count.buffer, 0, sizeof(uint32_t), 0);
        
            
            VkBufferMemoryBarrier barrier[]{
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, 
                    .pNext = nullptr,
                    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                    .buffer = resource.mesh.count.buffer,
                    .size = VK_WHOLE_SIZE
                }
            };
            vkCmdPipelineBarrier(pass.mesh_culling.cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, barrier, 0, nullptr);
        
            vkCmdBindPipeline(pass.mesh_culling.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, window.pass.mesh_culling.pipeline);

            vkCmdBindDescriptorSets(pass.mesh_culling.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, window.pass.mesh_culling.pipeline_layout, 0, 1, &pass.mesh_culling.descriptor, 0, nullptr);

            vkCmdDispatch(pass.mesh_culling.cmd, resource.mesh.meshlet_count, 1, 1); 

            vkEndCommandBuffer(pass.mesh_culling.cmd);
        }

        { // depth pass record
            VkCommandBufferBeginInfo begin{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = 0,
                .pInheritanceInfo = nullptr,
            };
            vkBeginCommandBuffer(pass.depth.cmd, &begin);

            VkBufferMemoryBarrier buf_barriers1[2]{
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                    .buffer = resource.mesh.indirect.buffer,
                    .size = VK_WHOLE_SIZE
                },
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                    .buffer = resource.mesh.count.buffer,
                    .size = VK_WHOLE_SIZE
                }
            };
            vkCmdPipelineBarrier(pass.depth.cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, 2, buf_barriers1, 0, nullptr);

            VkImageMemoryBarrier img_barriers1[1]{
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0,
                    .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, 
                    .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    .image = resource.attachment.depth.image,
                    .subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }
                }
            };
            vkCmdPipelineBarrier(pass.depth.cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, img_barriers1);
            
            VkRenderingAttachmentInfo depth_attachment{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = resource.attachment.depth.view,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = { .depthStencil = { .depth = 1.0f } }
            };
            VkRenderingInfo render{
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = { { 0, 0 }, { width, height } },
                .layerCount = 1,
                .pDepthAttachment = &depth_attachment
            };

            vkCmdBeginRendering(pass.depth.cmd, &render);

            vkCmdBindPipeline(pass.depth.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, window.pass.depth.pipeline);

            vkCmdBindDescriptorSets(pass.depth.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, window.pass.depth.pipeline_layout, 0, 1, &resource.mesh.draw_set, 0, nullptr);

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(pass.depth.cmd, 0, 1, &window.resource.mesh.position.buffer, &offset);

            vkCmdBindIndexBuffer(pass.depth.cmd, window.resource.mesh.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexedIndirectCount(pass.depth.cmd, resource.mesh.indirect.buffer, 0, resource.mesh.count.buffer, 0, max_draws, sizeof(VkDrawIndexedIndirectCommand));

            vkCmdEndRendering(pass.depth.cmd);

            VkImageMemoryBarrier img_barriers2[1]{
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
                    .image = resource.attachment.depth.image,
                    .subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }
                }
            };

            vkCmdPipelineBarrier(pass.depth.cmd, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, img_barriers2);

            vkEndCommandBuffer(pass.depth.cmd);
        }

        { // light cull record

        }

        { // forward pass record

        }
    }

}

void Arawn::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) { 
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    //wnd.template on<Key::Event>().invoke({ key, action });
}

void Arawn::mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) { 
    double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    glfwGetCursorPos(window, &x, &y);
    //wnd.template on<Mouse::Event>().invoke({ Mouse::SCROLL, yoffset > 0 ? Mouse::UP : Mouse::DOWN, static_cast<int>(x), static_cast<int>(y) });
}

void Arawn::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) { 
    double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    glfwGetCursorPos(window, &x, &y);
    //wnd.template on<Mouse::Event>().invoke({ Mouse::NONE, 0, static_cast<int>(x), static_cast<int>(y) });
}

void Arawn::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) { 
    double x, y;
    auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
    glfwGetCursorPos(window, &x, &y);
    //wnd.template on<Mouse::Event>().invoke({ button, action, static_cast<int>(x), static_cast<int>(y) });
}

const GLFWvidmode* Arawn::selectVideoMode(GLFWmonitor* monitor, int width, int height) {
    int count;
    const GLFWvidmode* cur = glfwGetVideoModes(monitor, &count);
    const GLFWvidmode* end = cur + count;
    const GLFWvidmode* min = nullptr;
    // find minimum video mode greater than or equal to specified resolution
    do {
        if (cur->width < width) continue;
        if (cur->height < height) continue;
        min = cur;
        break;
    } while(++cur < end);
    do {
        if (cur->width < width) continue;
        if (cur->height < height) continue;
        if (cur->width > min->width) continue;
        if (cur->height > min->height) continue;
        min = cur;
    } while(++cur < end);
    return min;
}

VkFormat Arawn::selectImageFormat(VkImageTiling tiling, VkFormatFeatureFlags features, const std::vector<VkFormat>& candidates) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(engine.gpu, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    assert(false && "failed to find match format. no supported candidates. ");

    return VK_FORMAT_UNDEFINED;
}