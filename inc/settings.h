#pragma once
#include <glm/glm.hpp>
#include <filesystem>
#include <string>

// window settings 
// * resolution
// * display mode

// swapchain settings
// * vsync_mode { OFF, ON }
// * sync_mode { DOUBLE, TRIPLE }
// * anti_alias { NONE, MSAA_2, MSAA_4, MSAA_8 }

// renderer
// * depth_mode { NONE, ENABLED, PREPASS };
// * shadow_mode { NONE, MAPPED, PROJECTION };
// * reflection { NONE, SCREENSPACE, };
// * SSAO { OFF ON }; // further parameters???

enum class DisplayMode { 
    WINDOWED=0,
    FULLSCREEN=1,
    EXCLUSIVE=2
};

enum class SyncMode {  
    DOUBLE=2,
    TRIPLE=3
};

enum class VsyncMode { 
    OFF,            // IMMEDIATE/FIFO_RELAXED
    ON              // FIFO/MAILBOX
};

enum class AntiAlias { 
    NONE = 1,       // VK_SAMPLE_COUNT_1
    MSAA_2 = 2,     // VK_SAMPLE_COUNT_2
    MSAA_4 = 4,     // VK_SAMPLE_COUNT_4
    MSAA_8 = 8,     // VK_SAMPLE_COUNT_8
    MSAA_16 = 16    // VK_SAMPLE_COUNT_16
};

enum class AspectRatio { 
    SQUARE,         // 1:1
    STANDARD,       // 4:3 
    WIDESCREEN,     // 16:9
    CINEMA,         // 21:9
};

struct Settings {
    Settings(std::filesystem::path fp);    
    void save(std::filesystem::path fp);

    // engine
    std::string device;
    
    // window
    glm::uvec2  resolution;
    DisplayMode display_mode;
    
    // swapchain
    SyncMode    sync_mode;
    VsyncMode   vsync_mode;
    AntiAlias   anti_alias;
};

extern Settings settings;