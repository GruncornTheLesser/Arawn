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

enum class AspectRatio { 
    SQUARE = 9,      //  1:1 = 9:9 = 1
    STANDARD = 12,   //  3:4 = 12:9 = 1.333...
    WIDESCREEN = 16, // 16:9 = 16:9 = 1.777...
    CINEMA = 21,     // 21:9 = 21:9 = 2.333...
    // generate aspect ratio by aspect_ratio / AspectRatio::Square
};

enum class RenderMode {
    FORWARD,        // standard object to screen rendering
    DEFERRED,       // separate geometry and shading calculations
};

enum class CullMode {
    NONE,           // no light culling
    TILED,          // light culled against tiled(x, y) bounding box
    CLUSTERED,      // light culled against clustered(x, y, z) bounding box
};

enum class DepthMode {
    NONE,
    PARALLEL,
    PRE_PASS
};


struct Settings {
    Settings(std::filesystem::path fp);    
    void save(std::filesystem::path fp);

    // engine
    std::string device;
    
    // window
    glm::uvec2 resolution;
    DisplayMode display_mode;
    
    // swapchain
    uint32_t    frame_count;
    VsyncMode   vsync_mode;
    AntiAlias   anti_alias;

    // renderer
    RenderMode render_mode;
    CullMode   cull_mode;
    DepthMode  depth_mode;

    

};

extern Settings settings;