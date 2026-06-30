#pragma once


#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <iostream>
#include <string_view>

#define ARAWN_LOG(MESSAGE) std::cout << __FILE__ << ":" << __LINE__ << " " << __PRETTY_FUNCTION__ << ":" << MESSAGE << std::endl;

#define ARAWN_ASSERT(STATEMENT, MESSAGE) if (STATEMENT) { } else [[unlikely]] { throw std::runtime_error(std::format("{}:{} {}:\n{}", __FILE__, __LINE__, __PRETTY_FUNCTION__, MESSAGE)); }

#define VK_ASSERT(STATEMENT) ARAWN_ASSERT(VkResult result = (STATEMENT); result == VK_SUCCESS, string_VkResult(result))
