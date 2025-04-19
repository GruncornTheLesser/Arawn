#pragma once
#include "vulkan.h"
#include <filesystem>
#include <string>

enum class DisplayMode : uint32_t { WINDOWED=0, FULLSCREEN=2, EXCLUSIVE=3, MASK=3 };
enum class VsyncMode : uint32_t { DISABLED=0, ENABLED=4, MASK=4 };
enum class RenderMode : uint32_t { FORWARD=0, DEFERRED=8, MASK=8 };
enum class CullingMode : uint32_t { NONE=0, TILED=16, CLUSTERED=48, MASK=48 };
enum class DepthMode : uint32_t { DISABLED=0, ENABLED=64, MASK=64 };
enum class AntiAlias : uint32_t { NONE=0, MSAA_2=128, MSAA_4=256, MSAA_8=384, MASK=384 };

struct Configuration {
    RenderMode render_mode() const;
    DisplayMode display_mode() const;
    CullingMode culling_mode() const;
    AntiAlias anti_alias_mode() const;
    DepthMode depth_mode() const;
    VsyncMode vsync_mode() const;
    
    bool vsync_enabled() const;
    bool msaa_enabled() const;
    bool culling_enabled() const;
    bool depth_prepass_enabled() const;
    bool deferred_pass_enabled() const;
    VK_ENUM(VkSampleCountFlagBits) sample_count() const;    

    uint32_t flags;
};

struct Settings : Configuration {
    Settings(std::filesystem::path fp);
    //void save(std::filesystem::path fp);

    // engine
    std::string device;
    glm::uvec2  resolution;
    float       aspect_ratio;
    uint32_t    frame_count;
};

extern Settings settings;