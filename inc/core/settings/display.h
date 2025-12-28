#pragma once
#include <cstdint>
#include <util/json.h>

namespace Arawn {
    struct DeviceName {
        static constexpr const char* NAME = "device name";

        DeviceName() = default;
        DeviceName(Json::String val);

        std::string data;
    };

    struct Resolution {
        static constexpr const char* NAME = "resolution";
        
        Resolution() = default;
        Resolution(Json::IntBuffer val);

        struct { 
            uint32_t width = 800;
            uint32_t height = 600;
        } data;
    };

    struct DisplayMode { 
        enum Enum : uint32_t {
            WINDOWED   = 0b0000'0000'0000'0000, 
            FULLSCREEN = 0b0000'0000'0000'0001, 
            EXCLUSIVE  = 0b0000'0000'0000'0010, 
        };
        static constexpr uint32_t MASK = 0b0000'0000'0000'0011;
        static constexpr const char* NAME = "display mode";
        
        DisplayMode() = default;
        DisplayMode(Json::String val);

        uint32_t data = WINDOWED;
    };

    struct FrameCount {
        enum Enum : uint32_t {
            DOUBLE_BUFFERED = 0b0000'0000'0000'0000,
            TRIPLE_BUFFERED = 0b0000'0000'0100'0000,
        };
        static constexpr uint32_t MASK = 0b0000'0000'0100'0000;
        static constexpr const char* NAME = "triple buffered";
        
        FrameCount() = default;
        FrameCount(Json::String val);

        uint32_t data = DOUBLE_BUFFERED;
    };

    struct VsyncMode { 
        enum Enum : uint32_t {

        DISABLED = 0b0000'0000'0000'0000, 
        ENABLED  = 0b0000'0000'0000'0100,
        }; 
        static constexpr uint32_t MASK = 0b0000'0000'0000'0100;
        static constexpr const char* NAME = "vsync";
        
        VsyncMode() = default;
        VsyncMode(Json::Boolean val);

        uint32_t data = DISABLED;
    };

    struct LowLatency { 
        enum Enum : uint32_t {
            DISABLED  = 0b0000'0000'0000'0000, 
            ENABLED   = 0b0000'0000'0000'1000,
        }; 
        static constexpr uint32_t MASK = 0b0000'0000'0000'1000;
        static constexpr const char* NAME = "low latency";
        
        LowLatency() = default;
        LowLatency(Json::Boolean val);

        uint32_t data = DISABLED;
    };

    struct AntiAlias { 
        enum Enum : uint32_t {
            DISABLED = 0b0000'0000'0000'0000, 
            MSAA_2   = 0b0000'0000'0001'0000, 
            MSAA_4   = 0b0000'0000'0010'0000,
            MSAA_8   = 0b0000'0000'0011'0000,
        }; 
        static constexpr uint32_t MASK = 0b0000'0000'0011'0000;
        static constexpr const char* NAME = "display mode";
        
        AntiAlias() = default;
        AntiAlias(Json::String val);

        uint32_t data = DISABLED;
    };




}