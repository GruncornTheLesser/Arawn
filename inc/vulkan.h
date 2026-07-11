#pragma once

#ifdef ARAWN_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <iostream>
#include <format>

#define VK_ENUM(ENUM) ENUM
#define VK_TYPE(TYPE) TYPE

#ifdef ARAWN_DEBUG
#define ARAWN_LOG_MESSAGE(LEVEL, MESSAGE) std::format("[{}] - {}:{} - {}(): {}", #LEVEL, __FILE__, __LINE__, __func__, MESSAGE)
#else
#define ARAWN_LOG_MESSAGE(LEVEL, MESSAGE) std::format("[{}]: {}", #LEVEL, MESSAGE)
#endif

#define ARAWN_LOG_IMPL(LEVEL, STREAM, MSG) STREAM << ARAWN_LOG_MESSAGE(LEVEL, MSG) << std::endl;

#ifdef ARAWN_LOG_VERBOSE
#define ARAWN_LOG_VERBOSE_IMPL(MSG) ARAWN_LOG_IMPL(VERBOSE, std::cout, MSG)
#else
#define ARAWN_LOG_VERBOSE_IMPL(MSG)
#endif

#ifdef ARAWN_LOG_DEBUG
#define ARAWN_LOG_DEBUG_IMPL(MSG) ARAWN_LOG_IMPL(DEBUG, std::cout, MSG)
#else
#define ARAWN_LOG_DEBUG_IMPL(MSG)
#endif

#ifdef ARAWN_LOG_WARNING
#define ARAWN_LOG_WARNING_IMPL(MSG) ARAWN_LOG_IMPL(WARNING, std::cout, MSG)
#else
#define ARAWN_LOG_WARNING_IMPL(MSG)
#endif

#ifdef ARAWN_LOG_ERROR
#define ARAWN_LOG_ERROR_IMPL(MSG) ARAWN_LOG_IMPL(ERROR, std::cout, MSG)
#else
#define ARAWN_LOG_ERROR_IMPL(MSG)
#endif

#define ARAWN_LOG(LEVEL, MESSAGE) ARAWN_LOG_##LEVEL##_IMPL(MESSAGE)

#define ARAWN_ASSERT(STATEMENT, MESSAGE) if (STATEMENT) { } else [[unlikely]] { throw std::runtime_error(ARAWN_LOG_MESSAGE(ERROR, MESSAGE)); }
#define VK_ASSERT(STATEMENT) ARAWN_ASSERT(VkResult statement_result = STATEMENT; statement_result == VK_SUCCESS, string_VkResult(statement_result))

#else
#define VK_ENUM(ENUM) uint32_t
#define VK_TYPE(TYPE) void*
#endif