#define ARAWN_IMPLEMENTATION
#include "settings.h"
#include "json.h"
#include <fstream>
#include <string>
#include <iostream>
#include <charconv>

Settings::Settings(std::filesystem::path fp) {
    std::ifstream file(fp, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("could not open file");

    std::string buffer(static_cast<std::size_t>(file.tellg()), '\0');

    file.seekg(0);
    file.read(buffer.data(), buffer.size());
    file.close();

    flags = 0;

    Json::Object settings = Json(buffer);

    try { // parse device
        device = settings["device"];
    } catch (Json::ParseException e) { 
        device = "";
    }
   
    try { // parse display mode
        std::string_view display_str = settings["display mode"];
        
        if      (display_str == "windowed") flags |= static_cast<uint32_t>(DisplayMode::WINDOWED);
        else if (display_str == "exclusive") flags |= static_cast<uint32_t>(DisplayMode::EXCLUSIVE); 
        else if (display_str == "fullscreen") flags |= static_cast<uint32_t>(DisplayMode::FULLSCREEN);
    
    } catch (Json::ParseException e) { 
        flags |= static_cast<uint32_t>(DisplayMode::WINDOWED);
    }

    try { // parse aspect ratio
        Json::String res = settings["aspect ratio"];
        size_t pos = res.find(':');
        int x, y;
        if (std::from_chars(res.begin(), res.begin() + pos,   x).ec != std::errc{}) throw Json::ParseException();
        if (std::from_chars(res.begin() + pos + 1, res.end(), y).ec != std::errc{}) throw Json::ParseException();
        
        aspect_ratio = float(x) / y;
    } catch(Json::ParseException e) {
        aspect_ratio = float(resolution.x) / resolution.y;
    }

    try { // parse resolution
        Json::IntBuffer res = settings["resolution"];
        resolution = { res[0], res[1] };
    } catch (Json::ParseException e) { 
        resolution = { 800, 600 };
    }

    try { // parse frame count
        frame_count = std::min<uint32_t>(settings["frame count"], MAX_FRAMES_IN_FLIGHT);    
    } catch (Json::ParseException e) { 
        frame_count = 2; // default 2
    }

    try { // parse vsync mode
        flags |= static_cast<uint32_t>((bool)settings["vsync"] ? VsyncMode::ENABLED : VsyncMode::DISABLED);
    } catch (Json::ParseException e) { }

    try { // parse anti alias mode
        std::string_view anti_alias_str = settings["anti alias"];

        if      (anti_alias_str == "none")   flags |= static_cast<uint32_t>(AntiAlias::NONE);
        else if (anti_alias_str == "msaa2")  flags |= static_cast<uint32_t>(AntiAlias::MSAA_2); 
        else if (anti_alias_str == "msaa4")  flags |= static_cast<uint32_t>(AntiAlias::MSAA_4); 
        else if (anti_alias_str == "msaa8")  flags |= static_cast<uint32_t>(AntiAlias::MSAA_8); 
        else throw Json::ParseException{};

    } catch (Json::ParseException e) { }

    try { // parse z pre pass enabled
        flags |= static_cast<uint32_t>((bool)settings["z prepass"] ? DepthMode::ENABLED : DepthMode::DISABLED);;
    } catch (Json::ParseException) { }

    try { // parse render mode
        std::string_view render_mode_str = settings["render mode"];

        if      (render_mode_str == "forward")  flags |= static_cast<uint32_t>(RenderMode::FORWARD);
        else if (render_mode_str == "deferred") flags |= static_cast<uint32_t>(RenderMode::DEFERRED);
        else throw Json::ParseException{};
    } catch (Json::ParseException) { }

    try { // parse culling mode
        std::string_view cluster_mode_str = settings["culling mode"];
        
        if      (cluster_mode_str == "none")      flags |= static_cast<uint32_t>(CullingMode::NONE);
        else if (cluster_mode_str == "tiled")     flags |= static_cast<uint32_t>(CullingMode::TILED);
        else if (cluster_mode_str == "clustered") flags |= static_cast<uint32_t>(CullingMode::CLUSTERED);
        else throw Json::ParseException{};
    } catch (Json::ParseException e) { }

    if (depth_mode() == DepthMode::DISABLED && culling_mode() == CullingMode::TILED && render_mode() == RenderMode::FORWARD) {
        flags |= static_cast<uint32_t>(DepthMode::ENABLED);
    }
}

RenderMode Configuration::render_mode() const {
    return static_cast<RenderMode>(flags & static_cast<uint32_t>(RenderMode::MASK));
}
DisplayMode Configuration::display_mode() const {
    return static_cast<DisplayMode>(flags & static_cast<uint32_t>(DisplayMode::MASK));
}
CullingMode Configuration::culling_mode() const {
    return static_cast<CullingMode>(flags & static_cast<uint32_t>(CullingMode::MASK));
}
AntiAlias Configuration::anti_alias_mode() const {
    return static_cast<AntiAlias>(flags & static_cast<uint32_t>(AntiAlias::MASK));
}

VsyncMode Configuration::vsync_mode() const {
    return static_cast<VsyncMode>(flags & static_cast<uint32_t>(VsyncMode::MASK));
}
DepthMode Configuration::depth_mode() const {
    return static_cast<DepthMode>(flags & static_cast<uint32_t>(DepthMode::MASK));
}

VK_ENUM(VkSampleCountFlagBits) Configuration::sample_count() const {
    switch (anti_alias_mode()) {
    case AntiAlias::MSAA_2: return VK_SAMPLE_COUNT_2_BIT; break;
    case AntiAlias::MSAA_4: return VK_SAMPLE_COUNT_4_BIT; break;
    case AntiAlias::MSAA_8: return VK_SAMPLE_COUNT_8_BIT; break;
    default:                return VK_SAMPLE_COUNT_1_BIT; break;
    }
}


bool Configuration::msaa_enabled() const {
    return flags & static_cast<uint32_t>(AntiAlias::MASK);
}

bool Configuration::culling_enabled() const {
    return flags & static_cast<uint32_t>(CullingMode::MASK);
}

bool Configuration::vsync_enabled() const {
    return flags & static_cast<uint32_t>(VsyncMode::ENABLED);
}

bool Configuration::depth_prepass_enabled() const {
    return flags & static_cast<uint32_t>(DepthMode::ENABLED);
}

bool Configuration::deferred_pass_enabled() const {
    return flags & static_cast<uint32_t>(RenderMode::DEFERRED);
}

