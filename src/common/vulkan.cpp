/*
#define ARAWN_INCLUDE_VULKAN
#include "common/vulkan.h"
#include <fstream>
#include <vector>

VkShaderModule loadShaderModule(VkDevice device, const char* path) {
	auto spirvPath = std::format("res/{}.spv", path);
	std::ifstream file(spirvPath, std::ios::ate | std::ios::binary);
	ARAWN_ASSERT(file.is_open(), std::format("failed to open file: '{}'", spirvPath));

	std::size_t size = static_cast<std::size_t>(file.tellg());
	std::vector<uint32_t> code(size / sizeof(uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(code.data()), size);
	
	file.close();

	VkShaderModule module;
	VkShaderModuleCreateInfo moduleInfo {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code.size() * sizeof(uint32_t),
		.pCode = code.data()
	};
	
	VK_ASSERT(vkCreateShaderModule(device, &moduleInfo, nullptr, &module));	
	
	return module;
}

*/