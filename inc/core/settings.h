#pragma once
#include "vulkan.h"
#include <filesystem>
#include <string>
#include <bit>

#define BIT_OFFSET(PREV_MASK) std::bit_width(static_cast<uint32_t>(PREV_MASK))
#define BIT_FLAG(N) (N << OFFSET)
#define BIT_MASK(N) (((1ul << std::bit_width<uint32_t>(N - 1)) - 1) << OFFSET)

enum class DisplayMode : uint32_t { 
    OFFSET=BIT_OFFSET(0), WINDOWED=BIT_FLAG(0), 
    FULLSCREEN=BIT_FLAG(1), 
    EXCLUSIVE=BIT_FLAG(2), 
    MASK=BIT_MASK(3) 
};
enum class VsyncMode : uint32_t   { 
    OFFSET=BIT_OFFSET(DisplayMode::MASK), 
    DISABLED=BIT_FLAG(0), 
    ENABLED=BIT_FLAG(1), 
    MASK=BIT_MASK(2) 
};
enum class LowLatency : uint32_t  { 
    OFFSET=BIT_OFFSET(VsyncMode::MASK), 
    DISABLED=BIT_FLAG(0), 
    ENABLED=BIT_FLAG(1), 
    MASK=BIT_MASK(2) 
};
enum class AntiAlias : uint32_t   { 
    OFFSET=BIT_OFFSET(LowLatency::MASK), 
    DISABLED=BIT_FLAG(0), 
    MSAA_2=BIT_FLAG(1), 
    MSAA_4=BIT_FLAG(2), 
    MSAA_8=BIT_FLAG(3), 
    MASK=BIT_MASK(4) 
};
enum class RenderMode : uint32_t  { 
    OFFSET=BIT_OFFSET(AntiAlias::MASK), 
    FORWARD=BIT_FLAG(0), 
    DEFERRED=BIT_FLAG(1), 
    MASK=BIT_MASK(2) 
};
enum class CullingMode : uint32_t { 
    OFFSET=BIT_OFFSET(RenderMode::MASK), 
    DISABLED=BIT_FLAG(0), 
    TILED=BIT_FLAG(1), 
    CLUSTERED=BIT_FLAG(2), 
    MASK=BIT_MASK(3) 
};
enum class DepthMode : uint32_t   { 
    OFFSET=BIT_OFFSET(CullingMode::MASK), 
    DISABLED=BIT_FLAG(0), 
    ENABLED=BIT_FLAG(1), 
    MASK=BIT_MASK(2) 
};

//enum class MipmapMode : uint32_t  { OFFSET=BIT_OFFSET(DepthMode::MASK), DISABLED=BIT_FLAG(0), MIPMAP_2=BIT_FLAG(1), MIPMAP_3=BIT_FLAG(2), MIPMAP_4=BIT_FLAG(3),MIPMAP_5=BIT_FLAG(4),MIPMAP_6=BIT_FLAG(5), MIPMAP_7=BIT_FLAG(6), MIPMAP_8=BIT_FLAG(7), MASK=BIT_MASK(8) }; 
//enum class FilterMode : uint32_t  { OFFSET=BIT_OFFSET(MipmapMode::MASK), NEAREST=BIT_FLAG(0), BILINEAR=BIT_FLAG(1), TRILINEAR=BIT_FLAG(2), ANISOTROPIC=BIT_FLAG(3), MASK=BIT_MASK(4) };

struct Configuration {
    DisplayMode display_mode() const;
    VsyncMode vsync_mode() const;
    LowLatency low_latency() const;
    AntiAlias anti_alias_mode() const;
    RenderMode render_mode() const;
    CullingMode culling_mode() const;
    DepthMode depth_mode() const;

    template<typename ... Ts> 
    bool is_enabled(Ts ... vals) const {
        return (static_cast<uint32_t>(vals) | ...) == (flags & (static_cast<uint32_t>(Ts::MASK) | ...));
    }

    void set_display_mode(DisplayMode val);
    void set_vsync_mode(VsyncMode val);
    void set_low_latency(LowLatency val);
    void set_anti_alias_mode(AntiAlias val);
    void set_render_mode(RenderMode val);
    void set_culling_mode(CullingMode val);
    void set_depth_mode(DepthMode val);
    
    bool vsync_enabled() const;
    bool low_latency_enabled() const;
    bool msaa_enabled() const;
    VK_ENUM(VkSampleCountFlagBits) sample_count() const;    
    bool deferred_pass_enabled() const;
    bool culling_enabled() const;
    bool depth_prepass_enabled() const;

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