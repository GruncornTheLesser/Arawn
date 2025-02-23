#pragma once
#include "vulkan.h"
#include "window.h"
#include <vector>
#include <span>
#include <filesystem>

enum class SyncMode { DOUBLE=2, TRIPLE=3 }; 
enum class VsyncMode { OFF, ON }; // ADAPTIVE
enum class AntiAlias { NONE, MSAA_2, MSAA_4, MSAA_8, MSAA_16 }; // FXAA_2, FXAA_4, FXAA_8, FXAA_16, 

class Pass {
public:
    virtual std::span<VK_TYPE(VkSemaphore)> submit(uint32_t frame_index) const = 0;
    virtual VK_TYPE(VkCommandBuffer) get_cmd_buffer() const = 0;
};

class Renderer {
public:
    struct Settings {
        VsyncMode vsync_mode;               // present mode
        AntiAlias anti_alias_mode;          // anti-alias mode
        SyncMode sync_mode;                 // double/triple buffering
    };
    Renderer(const Window& wnd, const Settings& settings);

    Renderer(const Window& wnd, 
        AntiAlias anti_alias=AntiAlias::NONE, 
        VsyncMode vsync_mode=VsyncMode::OFF, 
        SyncMode sync_mode=SyncMode::DOUBLE);
    
    ~Renderer();
    Renderer(Renderer&&);
    Renderer& operator=(Renderer&&);
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // updates the swapchain
    void set_vsync_mode(VsyncMode mode);
    VsyncMode get_vsync_mode() const;
    std::vector<VsyncMode> enum_vsync_modes() const;
    
    void set_sync_mode(SyncMode mode);
    SyncMode get_sync_mode() const;
    std::vector<SyncMode> enum_sync_modes() const;
    
    // updates the present pass
    void set_anti_alias(AntiAlias mode);
    AntiAlias get_anti_alias() const;
    std::vector<AntiAlias> enum_anti_alias() const;

    //void set_depth_prepass_enabled(bool value);
    //bool get_depth_prepass_enabled() const;

    void draw();

private:
    static inline auto get_sample_count(AntiAlias anti_alias) -> VK_TYPE(VkSampleCountFlagBits);
    static inline auto get_surface_data(VK_TYPE(VkSurfaceKHR) surface, VsyncMode vsync_mode)
     -> std::tuple<VK_TYPE(VkFormat), VK_TYPE(VkColorSpaceKHR), VK_ENUM(VkPresentModeKHR)>;

    static inline auto create_shader_module(std::filesystem::path fp) -> VK_TYPE(VkShaderModule);

    void init_descriptors();
    void destroy_descriptors();

    void init_present_commands();
    void destroy_present_commands();

    void init_swapchain(VK_TYPE(VkFormat) format, VK_TYPE(VkColorSpaceKHR) colour_space, VK_TYPE(VkPresentModeKHR) present_mode, VK_TYPE(VkSwapchainKHR) old_swapchain=nullptr);
    void destroy_swapchain();

    void init_present_framebuffer(VK_TYPE(VkFormat) depth_format, VK_TYPE(VkFormat) colour_format, VK_TYPE(VkSampleCountFlagBits) msaa_sample);
    void destroy_present_framebuffer();

    void recreate_swapchain();
private:
    const Window& window;
    struct { uint32_t x, y; 
        auto& operator=(std::pair<uint32_t, uint32_t> val) { x = val.first; y = val.second; return *this; }
    } resolution;
    VsyncMode vsync_mode;
    AntiAlias anti_alias;
    uint32_t frame_count; // double/triple buffering

    VK_TYPE(VkSurfaceKHR) surface;
    VK_TYPE(VkSwapchainKHR) swapchain;
    VK_TYPE(VkImageView)* swapchain_views;
    
    VK_TYPE(VkDescriptorSetLayout) set_layout;
    

    uint32_t frame_index; // current frame index
    struct {
        VK_TYPE(VkCommandBuffer) cmd_buffers[MAX_FRAMES_IN_FLIGHT];
        VK_TYPE(VkSemaphore)     finished[MAX_FRAMES_IN_FLIGHT];
        VK_TYPE(VkSemaphore)     image_available[MAX_FRAMES_IN_FLIGHT];
        VK_TYPE(VkFence)         in_flight[MAX_FRAMES_IN_FLIGHT];

        VK_TYPE(VkRenderPass) renderpass;
        VK_TYPE(VkFramebuffer)* framebuffers;
        struct {
            VK_TYPE(VkImage) image;
            VK_TYPE(VkImageView) view;
            VK_TYPE(VkDeviceMemory) memory;
        } colour_attachment, depth_attachment;

        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkPipelineLayout) layout;
    
    } present_pass;
};



/*
cmd_buffer -> 1 unique cmd_buffer per operation in parallel
semaphore -> signals for GPU to GPU, across multiple queues
fence -> signals for GPU to CPU, 
*/