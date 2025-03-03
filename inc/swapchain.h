#pragma once
#include "vulkan.h"
#include "window.h"
#include <vector>
#include <filesystem>
/*
enum class SyncMode { DOUBLE=2, TRIPLE=3 }; 
enum class VsyncMode { OFF, ON }; // ADAPTIVE
enum class AntiAlias { NONE = 1, MSAA_2 = 2, MSAA_4 = 4, MSAA_8 = 8, MSAA_16 = 16 }; // FXAA_2, FXAA_4, FXAA_8, FXAA_16, 

class Pass {
public:
    virtual std::span<VK_TYPE(VkSemaphore)> submit(uint32_t frame_index) const = 0;
    virtual VK_TYPE(VkCommandBuffer) get_cmd_buffer() const = 0;
};
*/
class Swapchain {
public:
    Swapchain();
    ~Swapchain();
    Swapchain(Swapchain&&);
    Swapchain& operator=(Swapchain&&);
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    // attributes
    void set_vsync_mode(VsyncMode mode);
    std::vector<VsyncMode> enum_vsync_modes() const;
    
    void set_sync_mode(SyncMode mode);
    std::vector<SyncMode> enum_sync_modes() const;
    
    void set_anti_alias(AntiAlias mode);
    std::vector<AntiAlias> enum_anti_alias() const;
    
    void draw();

    void recreate_swapchain();

private:
    static inline auto create_shader_module(std::filesystem::path fp) -> VK_TYPE(VkShaderModule);

    void init_descriptors();
    void destroy_descriptors();

    void init_present_commands();
    void destroy_present_commands();

private:

    VK_TYPE(VkSurfaceKHR) surface;
    VK_TYPE(VkSwapchainKHR) swapchain;
    VK_TYPE(VkImageView)* swapchain_views;  // array of image views   

    uint32_t frame_count = 0;
    uint32_t frame_index = 0; // current frame index
    VK_TYPE(VkCommandBuffer) cmd_buffers[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore)     render_finished_semaphore[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore)     image_available_semaphore[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkFence)         in_flight_fence[MAX_FRAMES_IN_FLIGHT];

    VK_TYPE(VkFramebuffer)* framebuffers; // array of framebuffers
    struct {
        VK_TYPE(VkImage) image;
        VK_TYPE(VkImageView) view;
        VK_TYPE(VkDeviceMemory) memory;
    } resolve_attachment, depth_attachment;



    VK_TYPE(VkRenderPass) renderpass;
    VK_TYPE(VkDescriptorSetLayout) set_layout;
    VK_TYPE(VkPipelineLayout) layout;
    VK_TYPE(VkPipeline) pipeline;
};

extern Swapchain swapchain;

/*
cmd_buffer -> 1 unique cmd_buffer per operation in parallel
semaphore -> signals for GPU to GPU, across multiple queues
fence -> signals for GPU to CPU, 
*/