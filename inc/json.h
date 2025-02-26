#pragma once
#include <vector>
#include <unordered_map>
#include <string_view>
#include <stdint.h>


struct Json {
    struct ParseException : std::exception { };
    using Integer = uint32_t;
    using Float = float;
    using Boolean = bool;
    using String = std::string_view;
    using Object = std::unordered_map<std::string_view, Json>;
    using Array = std::vector<Json>;

    Json() = default;
    Json(std::string_view view);

    operator Integer() const;
    operator Float() const;
    operator Boolean() const;
    operator String() const;
    operator Object() const;
    operator Array() const;

    Json operator[](const char* key) const; // cast as object and index
    Json operator[](int index) const; // cast as array and index
private:
    size_t ignore_whitespace(size_t pos) const;
    size_t end_of_string(size_t pos) const;
    size_t end_of_object(size_t pos, char delim) const;
    
    std::string_view view;
};
