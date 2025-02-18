#pragma once
#include "vulkan.h"
#include "window.h"
#include <vector>

enum class SyncMode { DOUBLE=2, TRIPLE=3 }; 
enum class VsyncMode { OFF, ON };
enum class AntiAlias { 
    NONE, 
    FXAA_2, FXAA_4, FXAA_8, FXAA_16, 
    MSAA_2, MSAA_4, MSAA_8, MSAA_16,
};

class Renderer {
public:
    struct Settings {
        struct { uint32_t x, y; } resolution;
        VsyncMode vsync_mode;               // present mode
        AntiAlias anti_alias_mode;          // anti-alias mode
        SyncMode sync_mode;                 // double/triple buffering
    };
    Renderer(const Window& wnd, const Settings& settings);

    Renderer(const Window& wnd, 
        uint32_t width, uint32_t height,
        AntiAlias anti_alias=AntiAlias::NONE, 
        VsyncMode vsync_mode=VsyncMode::OFF, 
        SyncMode sync_mode=SyncMode::DOUBLE);
    
    ~Renderer();
    Renderer(Renderer&&);
    Renderer& operator=(Renderer&&);
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // updates the swapchain
    void set_resolution(uint32_t width, uint32_t height);
    std::pair<uint32_t, uint32_t> get_resolution() const;
    std::vector<std::pair<uint32_t, uint32_t>> enum_resolutions() const;

    void set_vsync_mode(VsyncMode mode);
    VsyncMode get_vsync_mode() const;
    std::vector<VsyncMode> enum_vsync_modes() const;
    
    void set_sync_mode(SyncMode mode);
    SyncMode get_sync_mode() const;
    std::vector<SyncMode> enum_sync_modes() const;
    
    // updates the present pass
    void set_anti_alias(AntiAlias mode);
    AntiAlias get_anti_alias() const;
    std::vector<AntiAlias> enum_ant_alias() const;
    
    void draw();

private:
    const Window& window;
    
    struct { uint32_t x, y; } resolution;
    VsyncMode vsync_mode;
    AntiAlias anti_alias;
    uint32_t frame_count; // double/triple buffering
    uint32_t frame_index; // current frame index

    VK_TYPE(VkSurfaceKHR) surface;
    VK_TYPE(VkSwapchainKHR) swapchain;
    VK_TYPE(VkImageView)* swapchain_views;
    
    VK_TYPE(VkCommandBuffer) graphics_cmd_buffer[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkCommandBuffer) compute_cmd_buffer[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore)     image_available[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkSemaphore)     render_finished[MAX_FRAMES_IN_FLIGHT];
    VK_TYPE(VkFence)         in_flight[MAX_FRAMES_IN_FLIGHT];

    VK_TYPE(VkDescriptorSetLayout) layout; // layout for all 

    struct {
        struct {
            VK_TYPE(VkImage) image;
            VK_TYPE(VkImageView) view;
            VK_TYPE(VkDeviceMemory) memory;
        } attachment;
        
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkPipelineLayout) layout;
        VK_TYPE(VkRenderPass) renderpass;
        VK_TYPE(VkFramebuffer)* framebuffers;
    } present_pass;
};


/*
semaphore -> signals for GPU to GPU, across multiple queues
fence -> signals for GPU to CPU, 
*/