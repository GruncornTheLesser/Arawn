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
    
    Json::Object settings = Json(buffer);

    try { // parse device
        device = settings["device"];
    } catch (Json::ParseException e) { 
        device = "";
    }
   
    try { // parse display mode
        std::string_view display_str = settings["display mode"];
        
        if      (display_str == "windowed") display_mode = DisplayMode::WINDOWED;
        else if (display_str == "exclusive") display_mode = DisplayMode::EXCLUSIVE; 
        else if (display_str == "fullscreen") display_mode = DisplayMode::FULLSCREEN;
    
    } catch (Json::ParseException e) { 
        display_mode = DisplayMode::WINDOWED;
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
        bool vsync_enabled = settings["vsync mode"];
    } catch (Json::ParseException e) {
        vsync_enabled = false;
    }

    try { // parse anti alias mode
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

    try { // parse z pre pass enabled
        z_prepass_enabled = settings["z prepass"];
    } catch (Json::ParseException) {
        z_prepass_enabled = false;
    }

    try { // parse render mode
        std::string_view render_mode_str = settings["render mode"];

        if      (render_mode_str == "forward")  render_mode = RenderMode::FORWARD;
        else if (render_mode_str == "deferred") render_mode = RenderMode::DEFERRED;
        else throw Json::ParseException{};
    } catch (Json::ParseException) {
        render_mode = RenderMode::FORWARD;
    }

    try { // parse culling mode
        std::string_view cluster_mode_str = settings["culling mode"];
        
        if      (cluster_mode_str == "none")      culling_mode = CullingMode::NONE;
        else if (cluster_mode_str == "tiled")     culling_mode = CullingMode::TILED;
        else if (cluster_mode_str == "clustered") culling_mode = CullingMode::CLUSTERED;
        else throw Json::ParseException{};
    } catch (Json::ParseException e) {
        culling_mode = culling_mode = CullingMode::NONE;
    }

    try { // parse tile size
        Json::IntBuffer cluster = settings["cell size"];
        tile_size = { cluster[0], cluster[1] };
    } catch (Json::ParseException e) {
        tile_size = { 16, 16 };
    }

    culling_pass_enabled = culling_mode != CullingMode::NONE;
    deferred_pass_enabled = render_mode == RenderMode::DEFERRED;
    switch (anti_alias) {
    case AntiAlias::MSAA_2:  sample_count = VK_SAMPLE_COUNT_2_BIT; break;
    case AntiAlias::MSAA_4:  sample_count = VK_SAMPLE_COUNT_4_BIT; break;
    case AntiAlias::MSAA_8:  sample_count = VK_SAMPLE_COUNT_8_BIT; break;
    case AntiAlias::MSAA_16: sample_count = VK_SAMPLE_COUNT_16_BIT; break;
    default:                 sample_count = VK_SAMPLE_COUNT_1_BIT; break;
    }
    msaa_enabled = sample_count != VK_SAMPLE_COUNT_1_BIT;
}