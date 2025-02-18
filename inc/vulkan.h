#pragma once
#include <stdint.h>

#ifdef VK_IMPLEMENTATION
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_enum_string_helper.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string_view>
#include <source_location>
#include <iostream>

#define VK_TYPE(TYPE) TYPE
#define VK_ENUM(ENUM) ENUM
#define ENUM_ENTRY(NAME, ENUM) NAME = ENUM


void log_error(std::string_view msg, std::source_location loc = std::source_location::current());

#define VK_ASSERT(x) {                          \
    VkResult RESULT_VAL = (x);                  \
    if (RESULT_VAL != VK_SUCCESS) {             \
        log_error(string_VkResult(RESULT_VAL)); \
        throw std::exception();                 \
    }                                           \
}
#define GLFW_ASSERT(x) {                        \
    bool res = (x);                             \
    if (!res) {                                 \
        const char** desc;                      \
        glfwGetError(desc);                     \
        log_error(*desc);                       \
        throw std::exception();                 \
    }                                           \
} 

#else
#define VK_TYPE(TYPE) void*
#define VK_ENUM(ENUM) uint32_t
#define ENUM_ENTRY(NAME, ENUM) NAME
#endif

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT 3
#endif