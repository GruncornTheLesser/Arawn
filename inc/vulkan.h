#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

#ifdef ARAWN_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <iostream>
#include <format>

#define VK_ENUM(ENUM) ENUM
#define VK_TYPE(TYPE) TYPE

#ifdef ARAWN_DEBUG
#define ARAWN_LOG_MESSAGE(LEVEL, MESSAGE) std::format("[{}] - {}:{} - {}", #LEVEL, __FILE__, __LINE__, MESSAGE)
#else
#define ARAWN_LOG_MESSAGE(LEVEL, MESSAGE) std::format("[{}]: {}", #LEVEL, MESSAGE)
#endif

#define ARAWN_LOG_IMPL(LEVEL, STREAM, MESSAGE) STREAM << ARAWN_LOG_MESSAGE(LEVEL, MESSAGE) << std::endl;

#ifdef ARAWN_LOG_VERBOSE
#define ARAWN_LOG_VERBOSE_IMPL(MESSAGE) ARAWN_LOG_IMPL(VERBOSE, std::cout, MESSAGE)
#else
#define ARAWN_LOG_VERBOSE_IMPL(MESSAGE)
#endif

#ifdef ARAWN_LOG_INFO
#define ARAWN_LOG_INFO_IMPL(MESSAGE) ARAWN_LOG_IMPL(INFO, std::cout, MESSAGE)
#else
#define ARAWN_LOG_INFO_IMPL(MESSAGE)
#endif

#ifdef ARAWN_LOG_WARNING
#define ARAWN_LOG_WARNING_IMPL(MESSAGE) ARAWN_LOG_IMPL(WARNING, std::cout, MESSAGE)
#else
#define ARAWN_LOG_WARNING_IMPL(MESSAGE)
#endif

#ifdef ARAWN_LOG_ERROR
#define ARAWN_LOG_ERROR_IMPL(MESSAGE) ARAWN_LOG_IMPL(ERROR, std::cout, MESSAGE)
#else
#define ARAWN_LOG_ERROR_IMPL(MESSAGE)
#endif

#define ARAWN_LOG(LEVEL, MESSAGE) ARAWN_LOG_##LEVEL##_IMPL(MESSAGE)

#define ARAWN_THROW(MESSAGE) throw std::runtime_error(ARAWN_LOG_MESSAGE(ERROR, MESSAGE));
#define ARAWN_ASSERT(STATEMENT, MESSAGE) if (STATEMENT) { } else [[unlikely]] { ARAWN_THROW(MESSAGE) }

#define VK_ASSERT(STATEMENT) ARAWN_ASSERT(VkResult statement_result = STATEMENT; statement_result == VK_SUCCESS, string_VkResult(statement_result))


#define ARAWN_VERSION VK_MAKE_API_VERSION(ARAWN_VERSION_VARIANT, ARAWN_VERSION_MAJOR, ARAWN_VERSION_MINOR, ARAWN_VERSION_PATCH)

#ifndef ARAWN_VULKAN_VERSION
#define ARAWN_VULKAN_VERSION VK_API_VERSION_1_3
#endif

#else
#define VK_ENUM(ENUM) uint32_t
#define VK_TYPE(TYPE) void*
#endif