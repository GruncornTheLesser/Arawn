#pragma once
#include "vulkan.h"
#include <filesystem>
#include <string>

enum class DisplayMode { 
    WINDOWED=0,
    FULLSCREEN=1,
    EXCLUSIVE=2
};

enum class VsyncMode { 
    OFF,            // IMMEDIATE/FIFO_RELAXED
    ON              // FIFO/MAILBOX
};

enum class RenderMode {
    FORWARD,
    DEFERRED
};

enum class CullingMode {
    NONE,
    TILED,
    CLUSTERED
};

enum class AntiAlias { 
    NONE,       // VK_SAMPLE_COUNT_1
    MSAA_2,     // VK_SAMPLE_COUNT_2
    MSAA_4,     // VK_SAMPLE_COUNT_4
    MSAA_8,     // VK_SAMPLE_COUNT_8
    MSAA_16     // VK_SAMPLE_COUNT_16
    /*
    FXAA_2,
    FXAA_4,
    FXAA_8,
    FXAA_16,
    */
};

struct Settings {
    Settings(std::filesystem::path fp);
    //void save(std::filesystem::path fp);

    // engine
    std::string device;         // requires complete reinstancing of engine
    // swapchain
    DisplayMode display_mode;   // recreates window + swapchain + renderer
    glm::uvec2  resolution;      // recreates swapchain + renderer
    float       aspect_ratio;   // simply lists different set of resolutions
    uint32_t    frame_count;    // recreates renderer
    bool        vsync_enabled;  // recreates swapchain + renderer
    // renderer
    AntiAlias   anti_alias;     // recreates renderer
    bool z_prepass_enabled;     // recreates renderer
    RenderMode render_mode;     // recreates renderer
    CullingMode culling_mode;   // recreates renderer
    glm::uvec2 tile_size;       // reassigns uniform value

    bool culling_pass_enabled;
    bool deferred_pass_enabled;
    VK_ENUM(VkSampleCountFlagBits) sample_count;
    bool msaa_enabled;
    
};

extern Settings settings;