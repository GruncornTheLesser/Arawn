#define ARAWN_IMPLEMENTATION
#include <graphics/resources/program.h>
#include <graphics/engine.h>
#include <spirv_reflect.h>
#include <fstream>
#include <algorithm>

SpvReflectShaderModule loadShader(const char* filepath) {
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	if (!file.is_open()) throw std::runtime_error("failed to open shader file");

	std::vector<uint32_t> code;

	std::size_t fileSize = (std::size_t)file.tellg();
	code.resize(fileSize);

	file.seekg(0);
	file.read((char*)code.data(), fileSize);
	file.close();

	SpvReflectShaderModule module;

	SpvReflectResult result = spvReflectCreateShaderModule(code.size() * sizeof(uint32_t), code.data(), &module);

	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		throw std::runtime_error("failed to reflect spir-v shader");
	}

	return module;
}

VkPipelineLayout createLayout(const std::vector<const SpvReflectShaderModule*>& stages) {
	using namespace Arawn;
	using bindingMap_t = std::pair<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>;
	using setMap_t = std::vector<bindingMap_t>;
	setMap_t setMap;
	
	for (const auto* stage : stages) {
		uint32_t setCount;
		spvReflectEnumerateDescriptorSets(stage, &setCount, nullptr);
		
		std::vector<SpvReflectDescriptorSet*> sets(setCount);
		spvReflectEnumerateDescriptorSets(stage, &setCount, sets.data());

		for (uint32_t i = 0; i < sets.size(); ++i) {
			SpvReflectDescriptorSet* set = sets[i];
			uint32_t setIndex = set->set;

			auto it = std::find_if(setMap.begin(), setMap.end(), [=](const auto& setMap) { return setMap.first == setIndex; });
			if (it == setMap.end()) {
				setMap.push_back({ setIndex, {} });
			}
			auto& bindingMap = it->second;
			
			for (uint32_t j = 0; j < set->binding_count; ++j) {
				SpvReflectDescriptorBinding* binding = set->bindings[j];
				uint32_t bindingIndex = binding->binding;
				
				auto it = std::find_if(bindingMap.begin(), bindingMap.end(), [=](const auto& bindingData) { return bindingData.binding == bindingIndex; });
				if (it == bindingMap.end()) {
					bindingMap.push_back({ 
						.binding = bindingIndex,
						.descriptorType = static_cast<VkDescriptorType>(binding->descriptor_type),
						.descriptorCount = binding->count,
						.stageFlags = VK_SHADER_STAGE_ALL,
					});
				}
			}
		}
	}

	std::vector<VkDescriptorSetLayout> setLayouts;
	for (uint32_t i = 0; i < setMap.size(); ++i) {
		uint32_t setIndex = setMap[i].first;
		auto& bindingMap = setMap[i].second;
		
		setLayouts.emplace_back(engine.setLayout(bindingMap));
	}

	VkPipelineLayoutCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
		.pSetLayouts = setLayouts.data(),
	};

	VkPipelineLayout layout;
	VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &layout));
	return layout;
}

Arawn::Program::Program(const char* comp) {
	auto compModule = loadShader(comp);

	layout = createLayout(std::vector<const SpvReflectShaderModule*>{ &compModule });

	spvReflectDestroyShaderModule(&compModule);
}

Arawn::Program::Program(const char* vert, const char* frag) {
	auto vertModule = loadShader(vert);
	auto fragModule = loadShader(frag);	

	layout = createLayout(std::vector<const SpvReflectShaderModule*>{} = { &vertModule, &fragModule });

	spvReflectDestroyShaderModule(&vertModule);
	spvReflectDestroyShaderModule(&fragModule);
}

Arawn::Program::Program(const char* vert, const char* geom, const char* frag) {
	auto vertModule = loadShader(vert);
	auto geomModule = loadShader(geom);	
	auto fragModule = loadShader(frag);	

	layout = createLayout(std::vector<const SpvReflectShaderModule*>{} = { &vertModule, &geomModule, &fragModule });

	spvReflectDestroyShaderModule(&vertModule);
	spvReflectDestroyShaderModule(&geomModule);
	spvReflectDestroyShaderModule(&fragModule);
}

Arawn::Program::~Program() noexcept {
	vkDestroyPipeline(engine.device, pipeline, nullptr);
	vkDestroyPipelineLayout(engine.device, layout, nullptr);
}

Arawn::Program::Program(Program&& other) noexcept {
	pipeline = other.pipeline;
	layout = other.layout;

	other.layout = nullptr;
}

Arawn::Program& Arawn::Program::operator=(Program&& other) noexcept {
	if (pipeline != nullptr) {
		vkDestroyPipeline(engine.device, pipeline, nullptr);
		vkDestroyPipelineLayout(engine.device, layout, nullptr);
	}
	
	pipeline = other.pipeline;
	layout = other.layout;

	other.layout = nullptr;

	return *this;
}


