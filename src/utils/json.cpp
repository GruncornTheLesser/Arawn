#include "json.h"
#include <charconv>
#include <iostream>

// json standard 
// https://www.json.org/json-en.html
/*
design:
Json object is an opaque json value holding a string_view of the location in the buffer. 
On construction the end of the object is found:
 - ignore leading whitespace
 - comparing the first char, to evaluate the type
 - for arrays and objects, the closing deliminator, takes into account depth, strings and 
The data is accessed on casting the json type into the matching type. 
The raw string stored in the Json value is then converted into the value requested.

*/

Json::Json(std::string_view val) : view(val) {
    if (view.empty()) throw ParseException();
    
    size_t start = end_of_whitespace(0);

    switch (view.at(start)) {
    case 't': view = view.substr(start, 4); break;
    case 'f': view = view.substr(start, 5); break;
    case '"': view = view.substr(start, end_of_string(start) - start); break;
    case '[': view = view.substr(start, end_of_object(start, ']') - start); break;
    case '{': view = view.substr(start, end_of_object(start, '}') - start); break;
    default: view = view.substr(start, view.find_first_not_of("-1234567890.eE", start) - start); break;
    }
}

Json::operator Json::Integer() const {
    if (view.empty()) throw ParseException();

    size_t pos = view.find_first_not_of("0123456789+-");
    int val;
    if (std::from_chars(view.begin(), view.end(), val).ec != std::errc{}) throw ParseException();
    return val;
}

Json::operator Json::Float() const {
    if (view.empty()) throw ParseException();
    
    float val;
    if (std::from_chars(view.begin(), view.end(), val).ec != std::errc{}) throw ParseException();
    return val;
}

Json::operator Boolean() const {
    if (view.empty()) throw ParseException();

    if (view.substr(0, 4) == "true") return true;
    if (view.substr(0, 5) == "false") return false;

    throw ParseException();
}

Json::operator Json::String() const {
    if (view.empty()) throw ParseException();
    if (view.front() != '"') throw ParseException();
    return view.substr(1, view.size() - 2);
}

Json::operator Json::Object() const {
    if (view.empty()) throw ParseException();
    
    Json::Object object;
    size_t pos = 0;

    if (view[pos] != '{') throw ParseException();
    
    do {
        pos = end_of_whitespace(++pos);
        
        if (view[pos] == '}') break;

        Json field_name(view.substr(pos)); // parse string
        
        pos = end_of_whitespace(pos + field_name.view.size());

        if (view[pos] != ':') throw ParseException();
        
        pos = end_of_whitespace(pos + 1);

        Json field_value(view.substr(pos));
        object[field_name] = field_value;
        
        std::cout << field_name.view << " : " << field_value.view << std::endl;

        pos = end_of_whitespace(pos + field_value.view.size());
        
    } while (view[pos] == ',');

    return object;
}
template<typename T>
Json::operator Json::Buffer<T>() const {
    Json::Buffer<T> array;
    size_t pos = 0;
    
    if (view.empty()) throw ParseException();

    if (view[pos] != '[') throw ParseException();
    
    do {
        pos = end_of_whitespace(++pos);
        
        if (view[pos] == ']') break;

        Json element(view.substr(pos)); // parse string
        array.push_back(element);
        
        pos = end_of_whitespace(pos + element.view.size());
        
    } while (view[pos] == ',');

    return array;
}

template Json::operator Json::Array() const;
template Json::operator Json::Buffer<Json::Integer>() const;
template Json::operator Json::Buffer<Json::Float>() const;
template Json::operator Json::Buffer<Json::Boolean>() const;
template Json::operator Json::Buffer<Json::String>() const;

Json Json::operator[](const char* key) const {
    return static_cast<Json::Object>(*this)[key];
}

Json Json::operator[](int index) const {
    return static_cast<Json::Array>(*this)[index];
}

size_t Json::end_of_whitespace(size_t pos) const {
    pos = view.find_first_not_of(" \n\r\t", pos);

    while (view.substr(pos, 2) == "//") {
        pos = view.find_first_of('\n', pos + 2); // comment
        if (pos == std::string::npos) throw ParseException();
        
        pos = view.find_first_not_of(" \n\r\t", pos);
        if (pos == std::string::npos) throw ParseException();
    }

    return pos;
}

size_t Json::end_of_string(size_t pos) const {
    do {
        pos = view.find_first_of('"', pos + 1);
        if (pos == std::string::npos) throw ParseException();
    } while (view.at(pos - 1) == '\\');
    return pos + 1;
}

size_t Json::end_of_object(size_t pos, char delim) const {
    size_t depth = 1;
    do { 
        pos = view.find_first_of("\"/{}[]", pos + 1);
        if (pos == std::string::npos) throw ParseException();

        else if (view.at(pos) == '"') pos = end_of_string(pos); // string
        else if (view.at(pos) == '{') ++depth;
        else if (view.at(pos) == '}') --depth;
        else if (view.at(pos) == '[') ++depth;
        else if (view.at(pos) == ']') --depth;

    } while(depth != 0 || view.at(pos) != delim);
    
    return pos + 1;
}