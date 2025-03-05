#pragma once
#include <stdint.h>
#include <string_view>
#include <source_location>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

void log_error(std::string_view msg, std::source_location loc = std::source_location::current());

#ifdef VK_IMPLEMENTATION

#include <vulkan/vulkan.h> // for now 
#include <vulkan/vk_enum_string_helper.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

#define VK_TYPE(TYPE) TYPE
#define VK_ENUM(ENUM) ENUM
#define ENUM_ENTRY(NAME, ENUM) NAME = ENUM


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
#define MAX_FRAMES_IN_FLIGHT 4
#endif

#ifndef MAX_LIGHTS
#define MAX_LIGHTS 4096
#endif

#ifndef MAX_LIGHTS_PER_CLUSTER
#define MAX_LIGHTS_PER_CLUSTER 128
#endif

#ifndef MAX_LIGHTS_PER_TILE
#define MAX_LIGHTS_PER_TILE 256
#endif

#ifndef MAX_NUM_CLUSTERS
#define MAX_NUM_CLUSTERS 2048
#endif

#ifndef MAX_NUM_TILES
#define MAX_NUM_TILES 512
#endif