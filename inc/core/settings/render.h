#pragma once
#include <cstdint>
#include <string>
#include <util/json.h>

namespace Arawn {
    struct RenderMode {
        enum Enum : uint32_t {
            FORWARD  = 0b0000'0000'0000'0000, 
            DEFERRED = 0b0000'0000'0100'0000,
        };
        static constexpr uint32_t MASK = 0b0000'0000'0100'0000; 
        static constexpr const char* NAME = "deferred";

        RenderMode() = default;
        RenderMode(Json::Boolean val);

        uint32_t data = DEFERRED;
    };
    
    struct CullingMode { 
        enum Enum : uint32_t {
            DISABLED = 0b0000'0000'0000'0000, 
            TILE     = 0b0000'0000'1000'0000,
            CLUSTER  = 0b0000'0001'0000'0000,
        };

        static constexpr uint32_t MASK = 0b0000'0001'1000'0000;
        static constexpr const char* NAME = "culling mode";

        CullingMode() = default;
        CullingMode(Json::String val);

        uint32_t data = TILE;
    };
    
    struct DepthMode   { 
        enum Enum : uint32_t {
            DISABLED = 0b0000'0000'0000'0000, 
            ENABLED  = 0b0000'0010'0000'0000,
        };

        static constexpr uint32_t MASK = 0b0000'0000'0010'0000;
        static constexpr const char* NAME = "z pass";

        DepthMode() = default;
        DepthMode(Json::Boolean val);

        uint32_t data = ENABLED;
    };
    
    struct MipmapMode  { 
        enum Enum : uint32_t {
            DISABLED = 0b0000'0000'0000'0000, 
            MIPMAP_2 = 0b0000'1000'0000'0000,
            MIPMAP_3 = 0b0001'0000'0000'0000, 
            MIPMAP_4 = 0b0001'1000'0000'0000,
            MIPMAP_5 = 0b0010'0000'0000'0000,
            MIPMAP_6 = 0b0010'1000'0000'0000,
            MIPMAP_7 = 0b0011'0000'0000'0000,
            MIPMAP_8 = 0b0011'1000'0000'0000,
        };

        static constexpr uint32_t MASK = 0b0011'1000'0000'0000;
        static constexpr const char* NAME = "max mipmap";

        MipmapMode() = default;
        MipmapMode(Json::Integer val);

        uint32_t data = MIPMAP_4;
    };

    struct FilterMode  { 
        enum Enum : uint32_t {
            NEAREST     = 0b0000'0000'0000'0000, 
            BILINEAR    = 0b0100'0000'0000'0000,
            TRILINEAR   = 0b1000'0000'0000'0000,
            ANISOTROPIC = 0b1100'0000'0000'0000,
        };

        static constexpr uint32_t MASK = 0b1100'0000'0000'0000;
        static constexpr const char* NAME = "texture filter";

        FilterMode() = default;
        FilterMode(Json::String val);

        uint32_t data = BILINEAR;
    };
}