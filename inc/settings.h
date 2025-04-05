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
    std::string device;
    glm::uvec2 resolution;
    DisplayMode display_mode;
    float       aspect_ratio;
    uint32_t    frame_count;
    VsyncMode   vsync_mode;
    AntiAlias   anti_alias;
    bool z_prepass_enabled;
    RenderMode render_mode;
    glm::uvec3 cluster_count;

    bool culling_pass_enabled;
    bool deferred_pass_enabled;
    VK_ENUM(VkSampleCountFlagBits) sample_count;
    bool msaa_enabled;
    
};

extern Settings settings;