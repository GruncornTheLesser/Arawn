#pragma once
#include <graphics/vulkan.h>

namespace Arawn {
	class Program {
	public:
		Program(const char* compute);
		Program(const char* vertex, const char* fragment);
		Program(const char* vertex, const char* geometry, const char* fragment);

		~Program() noexcept;

		Program(const Program&) = delete;
		Program& operator=(const Program&) = delete;

		Program(Program&&) noexcept;
		Program& operator=(Program&&) noexcept;
	// private:
		VK_TYPE(VkPipeline) pipeline;
		VK_TYPE(VkPipelineLayout) layout;
	};
}


