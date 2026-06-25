#pragma once

#ifdef ARAWN_IMPLEMENTATION

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>

#ifdef ARAWN_DEBUG
#include <iostream>
#include <stdexcept>

#define LOG(x) { std::cout << x << std::endl; }

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

#define VK_TYPE(TYPE) TYPE
#define VK_ENUM(ENUM) ENUM
#define GLFW_WINDOW GLFWwindow*

#else
#include <cstdint>

#define VK_TYPE(TYPE) void*
#define VK_ENUM(ENUM) uint32_t
#define GLFW_WINDOW void*

#endif
