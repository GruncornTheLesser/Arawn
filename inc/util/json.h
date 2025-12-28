#pragma once
#include <vector>
#include <unordered_map>
#include <string_view>
#include <cstdint>
#include <filesystem>

struct Json {
    struct ParseException : std::exception { };
    using Integer = uint32_t;
    using Float = float;
    using Boolean = bool;
    using String = std::string_view;
    using Object = std::unordered_map<std::string_view, Json>;
    using Array = std::vector<Json>;
    template<typename T> using Buffer = std::vector<T>;
    using IntBuffer = Buffer<Integer>;
    using FloatBuffer = Buffer<Float>;
    using BooleanBuffer = Buffer<Boolean>;
    using StringBuffer = Buffer<String>;

    Json() = default;
    Json(std::string_view view);

    static std::string load(const std::filesystem::path& fp);

    operator Integer() const;
    operator Float() const;
    operator Boolean() const;
    operator String() const;
    operator Object() const;
    
    template<typename T>
    operator Buffer<T>() const; // only supports Int, Float, Boolean and String
    
    Json operator[](const char* key) const; // cast as object and index
    Json operator[](int index) const;       // cast as array and index
private:
    size_t endOfWhitespace(size_t pos) const;
    size_t endOfString(size_t pos) const;
    size_t endOfObject(size_t pos, char delim) const;
    
    std::string_view view;
};
