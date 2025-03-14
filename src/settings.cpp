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
    
    Json::Object settings;
    try {
        settings = Json(buffer);
    } catch (Json::ParseException e) {
        return;
    }
    
    try {
        device = settings["device"];
    } catch (Json::ParseException e) { 
        device = "";
    }

    try {
        Json::Array res = settings["resolution"];
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
        uint32_t frame_count = settings["frame count"];    
    } catch (Json::ParseException e) { 
        uint32_t frame_count = 2; // default 2
    }

    try {
        std::string_view vsync_str = settings["vsync mode"];
        
        if      (vsync_str == "off") vsync_mode = VsyncMode::OFF; 
        else if (vsync_str == "on") vsync_mode = VsyncMode::ON;

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

    } catch (Json::ParseException e) {
        anti_alias = AntiAlias::NONE;
    }
}