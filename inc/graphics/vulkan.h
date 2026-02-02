#pragma once
#include <cstdint>

#ifdef ARAWN_IMPLEMENTATION
#define VK_IMP(IMP, DEFAULT) IMP
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#ifdef ARAWN_DEBUG
#include <iostream>
#define LOG(x) { std::cerr << x << std::endl; }

#define VK_ASSERT(x) {                                      \
    VkResult RESULT_VAL = (x);                              \
    if (RESULT_VAL != VK_SUCCESS) {                         \
        const char* desc = string_VkResult(RESULT_VAL);     \
        LOG(__FILE__ << ":" << __LINE__ << " - " << desc)   \
        throw std::runtime_error(desc);                     \
    }                                                       \
}

#define GLFW_ASSERT(x) {                                    \
    bool res = (x);                                         \
    if (!res) {                                             \
        const char** desc;                                  \
        glfwGetError(desc);                                 \
        LOG(__FILE__ << ":" << __LINE__ << " - " << *desc)  \
        throw std::runtime_error(std::string{ *desc });     \
    }                                                       \
}

#else
#define LOG(x)

#define VK_ASSERT(x) {                                      \
    VkResult RESULT_VAL = (x);                              \
    if (RESULT_VAL != VK_SUCCESS) {                         \
        throw std::runtime_error("vk assert failed");       \
    }                                                       \
}

#define GLFW_ASSERT(x) {                                    \
    bool res = (x);                                         \
    if (!res) {                                             \
        throw std::runtime_error("glfw assert failed");     \
    }                                                       \
}

#endif

#define VK_TYPE(HANDLE) HANDLE
#define VK_ENUM(ENUM) ENUM
#define GLFW_WINDOW GLFWwindow*

#else
#define VK_TYPE(HANDLE) std::nullptr_t
#define VK_ENUM(ENUM) uint32_t
#define GLFW_WINDOW std::nullptr_t
#endif



#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT 3
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