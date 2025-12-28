#include <core/settings/render.h>

Arawn::RenderMode::RenderMode(Json::Boolean deferEnabled) : data(0) {
    if (deferEnabled) { data = DEFERRED; } else { data = FORWARD; }
}
Arawn::CullingMode::CullingMode(Json::String str) : data(0) {
    if (str == "tile") { data = TILE; } else
    if (str == "cluster") { data = CLUSTER; }
}
Arawn::DepthMode::DepthMode(Json::Boolean depthEnabled) : data(0) {
            if (depthEnabled) { data = ENABLED; } else { data = DISABLED; } 
} 
Arawn::MipmapMode::MipmapMode(Json::Integer mipmap) : data(0) {
    switch(mipmap) 
    {
        case 0:
        case 1: data = DISABLED; break;
        case 2: data = MIPMAP_2; break;
        case 3: data = MIPMAP_3; break;
        case 4: data = MIPMAP_4; break;
        case 5: data = MIPMAP_5; break;
        case 6: data = MIPMAP_6; break;
        case 7: data = MIPMAP_7; break;
        case 8: data = MIPMAP_8; break;
    }
}
Arawn::FilterMode::FilterMode(Json::String str) : data(0) {
    if (str == "nearest") { data = NEAREST;  } else
    if (str == "bilinear") { data = BILINEAR; } else
    if (str == "trilinear") { data = TRILINEAR; } else
    if (str == "anisotropic") { data = ANISOTROPIC; }
}