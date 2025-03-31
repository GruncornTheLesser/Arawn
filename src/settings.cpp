#define ARAWN_IMPLEMENTATION
#include "settings.h"
#include <fstream>
#include <string>
#include "json.h"
#include <iostream>

Settings::Settings(std::filesystem::path fp) {
    std::ifstream file(fp, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("could not open file");

    std::string buffer(static_cast<std::size_t>(file.tellg()), '\0');

    file.seekg(0);
    file.read(buffer.data(), buffer.size());
    file.close();
    
    Json::Object settings = Json(buffer);

    try {
        device = settings["device"];
    } catch (Json::ParseException e) { 
        device = "";
    }

    try {
        Json::IntBuffer res = settings["resolution"];
        resolution = { res[0], res[1] };
    } catch (Json::ParseException e) { 
        resolution = { 800, 600 };
    }

    try {
        std::string_view display_str = settings["display mode"];
        
        if      (display_str == "windowed") display_mode = DisplayMode::WINDOWED;
        else if (display_str == "exclusive") display_mode = DisplayMode::EXCLUSIVE; 
        else if (display_str == "fullscreen") display_mode = DisplayMode::FULLSCREEN;
    
    } catch (Json::ParseException e) { 
        display_mode = DisplayMode::WINDOWED;
    }

    try {
        uint32_t frame_count = std::min<int32_t>(settings["frame count"], MAX_FRAMES_IN_FLIGHT);    
    } catch (Json::ParseException e) { 
        uint32_t frame_count = 2; // default 2
    }

    try {
        std::string_view vsync_str = settings["vsync mode"];
        
        if      (vsync_str == "off") vsync_mode = VsyncMode::OFF; 
        else if (vsync_str == "on") vsync_mode = VsyncMode::ON;
        else throw Json::ParseException{};

    } catch (Json::ParseException e) {
        vsync_mode = VsyncMode::OFF;
    }

    try {
        std::string_view anti_alias_str = settings["anti alias"];

        if      (anti_alias_str == "none") anti_alias = AntiAlias::NONE;
        else if (anti_alias_str == "msaa2") anti_alias = AntiAlias::MSAA_2; 
        else if (anti_alias_str == "msaa4") anti_alias = AntiAlias::MSAA_4; 
        else if (anti_alias_str == "msaa8") anti_alias = AntiAlias::MSAA_8; 
        else if (anti_alias_str == "msaa16") anti_alias = AntiAlias::MSAA_16;
        else throw Json::ParseException{};

    } catch (Json::ParseException e) {
        anti_alias = AntiAlias::NONE;
    }

    try {
        z_prepass = settings["z prepass"];
    } catch (Json::ParseException) {
        z_prepass = false;
    }

    try {
        std::string_view render_mode_str = settings["render mode"];

        if (render_mode_str == "forward") render_mode = RenderMode::FORWARD;
        if (render_mode_str == "deferred") render_mode = RenderMode::DEFERRED;
        else throw Json::ParseException{};

    } catch (Json::ParseException) {
        z_prepass = false;
    }

    try {
        Json::IntBuffer cluster = settings["cluster count"];
        cluster_count = { 
            std::max<uint32_t>(1, cluster[0]),
            std::max<uint32_t>(1, cluster[1]),
            std::max<uint32_t>(1, cluster[2])
        };
    } catch (Json::ParseException e) {
        cluster_count = { 1, 1, 1 };
    }
}

bool Settings::z_prepass_enabled() const {
    return z_prepass;
}
bool Settings::cluster_pass_enabled() const {
    return settings.cluster_count.x > 1 || settings.cluster_count.y > 1 || settings.cluster_count.z > 1;
}
bool Settings::deferred_pass_enabled() const {
    return settings.render_mode == RenderMode::DEFERRED;
}
VkSampleCountFlagBits Settings::sample_count() const {
    switch (settings.anti_alias) {
    case AntiAlias::MSAA_2:  return VK_SAMPLE_COUNT_2_BIT;
    case AntiAlias::MSAA_4:  return VK_SAMPLE_COUNT_4_BIT;
    case AntiAlias::MSAA_8:  return VK_SAMPLE_COUNT_8_BIT;
    case AntiAlias::MSAA_16: return VK_SAMPLE_COUNT_16_BIT;
    default:                 return VK_SAMPLE_COUNT_1_BIT;
    }
}

bool Settings::msaa_enabled() const {
    return sample_count() != VK_SAMPLE_COUNT_1_BIT;
}
