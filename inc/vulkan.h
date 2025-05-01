#pragma once
#include <stdint.h>
#include <string_view>
#include <source_location>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

void log_error(std::string_view msg, std::source_location loc = std::source_location::current());

#ifdef ARAWN_IMPLEMENTATION

#include <vulkan/vulkan.h> // for now 
#include <vulkan/vk_enum_string_helper.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

#define VK_TYPE(TYPE) TYPE
#define VK_ENUM(ENUM) ENUM


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
#endif

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT 4
#endif

#ifndef MAX_LIGHTS
#define MAX_LIGHTS 4096
#endif

#ifndef MAX_LIGHTS_PER_CLUSTER
#define MAX_LIGHTS_PER_CLUSTER 63
#endif

#ifndef MAX_LIGHTS_PER_TILE
#define MAX_LIGHTS_PER_TILE 127
#endif

#ifndef MAX_MIPMAP_LEVEL 
#define MAX_MIPMAP_LEVEL 8
#endif