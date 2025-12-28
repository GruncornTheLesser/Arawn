#pragma once
#include <core/settings/display.h>
#include <core/settings/render.h>
#include <util/json.h>

#include <filesystem>

namespace Arawn {
    template<typename T> concept is_flag = requires { typename T::Enum; };

    template<typename ... Ts>
    struct Configuration { 
        template<typename T> static constexpr bool has_configuration_v = (std::is_same_v<T, Ts> || ...);
        template<typename T> static constexpr std::size_t data_index_v = []{ 
            std::size_t i = 0;
            bool _ = ((!is_flag<Ts> && (i++, std::is_same_v<T, Ts>)) || ...);
            return i - 1;
        }();
        
        using flag_type = std::conditional_t<(is_flag<Ts> || ...), uint32_t, std::type_identity<void>>;
        using data_type = decltype(std::tuple_cat(std::declval<std::conditional_t<is_flag<Ts>, std::tuple<>, std::tuple<decltype(Ts::data)>>>()...));

        Configuration(std::filesystem::path fp)
        {
            auto buffer = Json::load(fp);
            Json json(buffer);
            
            if constexpr (std::is_same_v<uint32_t, flag_type>)
            {
                flags = 0;
            }

            [&]<typename ... Us>(std::type_identity<std::tuple<Us...>>) {
                (void([&]() {
                    try
                    {
                        if constexpr (is_flag<Us>) 
                        {
                            set<Us>(static_cast<typename Us::Enum>(Us(json[Us::NAME]).data));
                        }
                        else
                        {
                            set<Us>(Us(json[Us::NAME]).data);
                        }
                    }
                    catch (Json::ParseException) { }
                }()), ...);                
            }(std::type_identity<std::tuple<Ts...>>{});
        }

        template<typename T> requires (has_configuration_v<T> && is_flag<T>)
        auto get() const {
            return static_cast<T::Enum>(flags & static_cast<uint32_t>(T::MASK));
        }

        template<typename T> requires (has_configuration_v<T> && !is_flag<T>)
        auto& get() {
            return std::get<data_index_v<T>>(data);
        }

        template<typename T> requires (has_configuration_v<T> && !is_flag<T>)
        const auto& get() const {
            return std::get<data_index_v<T>>(data);
        }

        template<typename T> requires (has_configuration_v<T> && is_flag<T>)
        void set(T::Enum value) {
            flags &= ~static_cast<uint32_t>(T::MASK);
            flags |= static_cast<uint32_t>(value);
        }

        template<typename T> requires (has_configuration_v<T> && !is_flag<T>)
        void set(const auto& value) {
            get<T>() = value;
        }

    private:
        [[no_unique_address]] flag_type flags;
        [[no_unique_address]] data_type data;
    };
    
    using Settings = Configuration<
        Resolution, DisplayMode, VsyncMode, LowLatency, AntiAlias, // display settings
        DeviceName, RenderMode, CullingMode, DepthMode, MipmapMode, FilterMode, FrameCount // render settings
    >;

    extern Settings settings;
}