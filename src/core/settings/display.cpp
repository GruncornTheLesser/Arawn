#include <core/settings/display.h>

Arawn::DeviceName::DeviceName(Json::String val) : data(val) { }

Arawn::DisplayMode::DisplayMode(Json::String val) {
    if (val == "fullscreen") { data = FULLSCREEN; } 
    else if (val == "exclusive") { data = EXCLUSIVE; } 
    else { data = WINDOWED; }
}

Arawn::VsyncMode::VsyncMode(Json::Boolean val) {
    if (val) { data = ENABLED; } else { data = DISABLED; }
}

Arawn::LowLatency::LowLatency(Json::Boolean val) {
    if (val) { data = ENABLED; } else { data = DISABLED; }
}

Arawn::AntiAlias::AntiAlias(Json::String val) {
    if      (val == "none")    { data = DISABLED; }
    else if (val == "msaa2")   { data = MSAA_2; }
    else if (val == "msaa4")   { data = MSAA_4; }
    else if (val == "msaa8")   { data = MSAA_8; }
}

Arawn::FrameCount::FrameCount(Json::String val) {
    if (val == "triple") { data = TRIPLE_BUFFERED; }
    else { data = DOUBLE_BUFFERED; }
}

Arawn::Resolution::Resolution(Json::IntBuffer val) {
    if (val.size() == 2) {
        data.width  = val[0];
        data.height = val[1];
    }
}