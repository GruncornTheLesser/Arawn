#pragma once
#include <stdint.h>

#ifdef VK_IMPLEMENTATION
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_enum_string_helper.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VK_TYPE(TYPE) TYPE
#define VK_ENUM(ENUM) ENUM
#define ENUM_ENTRY(NAME, ENUM) NAME = ENUM
#define VK_ASSERT(x) {                          \
VkResult res = x;                               \
if (res != VK_SUCCESS) {                        \
    std::string msg = "[Vulkan Error] line=";   \
    msg += std::to_string(__LINE__);            \
    msg += " - ";                               \
    msg += string_VkResult(res);                \
    throw std::runtime_error(msg);              \
    }                                           \
}
#else
#define VK_TYPE(TYPE) void*
#define VK_ENUM(ENUM) uint32_t
#define ENUM_ENTRY(NAME, ENUM) NAME
#endif

