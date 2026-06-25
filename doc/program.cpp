#define ARAWN_IMPLEMENTATION

/*
#include <render/resources/program.h>
#include <render/core/engine.h>
#include <spirv_reflect.h>
#include <algorithm>
#include <fstream>
#include <cassert>

std::vector<uint32_t> loadShaderCode(const char* filepath) {
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	if (!file.is_open()) throw std::runtime_error("failed to open shader file");
	
	std::vector<uint32_t> code;
	
	std::size_t fileSize = (std::size_t)file.tellg();
	code.resize(fileSize);
	
	file.seekg(0);
	file.read((char*)code.data(), fileSize);
	file.close();

	return code;
}

SpvReflectShaderModule createShaderReflect(const std::vector<uint32_t>& code) { 
	SpvReflectShaderModule reflect;
	auto result = spvReflectCreateShaderModule(code.size() * sizeof(uint32_t), code.data(), &reflect);
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		throw std::runtime_error("failed to reflect shader code");
	}
	return reflect;
}

VkShaderModule createShaderModule(const std::vector<uint32_t>& code) {
	 VkShaderModule module;
	 VkShaderModuleCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, 
		.codeSize = code.size(), 
		.pCode = code.data()
	};
	VK_ASSERT(vkCreateShaderModule(Arawn::engine.device, &info, nullptr, &module));
	return module;
}

VkPipelineLayout createPipelineLayout(const std::vector<SpvReflectShaderModule>& stages) {
	using namespace Arawn;
	std::vector<uint32_t> setIndices;
	std::vector<std::vector<VkDescriptorSetLayoutBinding>> setBindings;
	
	for (const auto& stage : stages) {
		uint32_t setCount;
		spvReflectEnumerateDescriptorSets(&stage, &setCount, nullptr);
		
		std::vector<SpvReflectDescriptorSet*> sets(setCount);
		spvReflectEnumerateDescriptorSets(&stage, &setCount, sets.data());

		for (uint32_t i = 0; i < sets.size(); ++i) {
			SpvReflectDescriptorSet* set = sets[i];
			uint32_t setIndex = set->set;

			std::size_t setMatch = std::find(setIndices.begin(), setIndices.end(), setIndex) - setIndices.begin();
			if (setMatch == setIndices.size()) {
				setIndices.push_back(setIndex);
				setBindings.push_back({ });
			}

			auto& bindings = setBindings[setMatch];
			
			for (uint32_t j = 0; j < set->binding_count; ++j) {
				SpvReflectDescriptorBinding* binding = set->bindings[j];
				uint32_t bindingIndex = binding->binding;
				
				auto it = std::find_if(bindings.begin(), bindings.end(), [=](const auto& bindingData) { 
					return bindingData.binding == bindingIndex;
				});

				if (it == bindings.end()) {
					bindings.push_back({ 
						.binding = bindingIndex,
						.descriptorType = static_cast<VkDescriptorType>(binding->descriptor_type),
						.descriptorCount = binding->count,
						.stageFlags = VK_SHADER_STAGE_ALL,
					});
				} else {
					assert(it->descriptorType != static_cast<VkDescriptorType>(binding->descriptor_type));
					assert(it->descriptorCount != static_cast<VkDescriptorType>(binding->count));
				}
			}
		}
	}

	std::vector<VkDescriptorSetLayout> setLayouts(setBindings.size());
	for (uint32_t i = 0; i < setLayouts.size(); ++i) {
		//setLayouts[i] = engine.setLayout(setMap[i].second);
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
	VkComputePipelineCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = { 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		},
		.layout = layout,
		.basePipelineHandle = nullptr,
		.basePipelineIndex = 0,
	};
	
	std::vector<SpvReflectShaderModule> reflect_info;
	auto compCode = loadShaderCode(comp);
	reflect_info.push_back(createShaderReflect(compCode));
	info.stage.module = createShaderModule(compCode);
	
	layout = createPipelineLayout(reflect_info);
	
	vkCreateComputePipelines(engine.device, nullptr, 1, &info, nullptr, &pipeline);

	spvReflectDestroyShaderModule(&reflect_info.back());
}

Arawn::Program::Program(VkRenderPass renderpass, const char* vert, const char* frag) : Program(renderpass, vert, nullptr, frag) { }

Arawn::Program::Program(VkRenderPass renderpass, const char* vert, const char* geom, const char* frag) {
	std::vector<VkPipelineShaderStageCreateInfo> stageInfo;
	std::vector<SpvReflectShaderModule> reflect_info;
	
	{ // load vertex shader
		auto vertCode = loadShaderCode(vert);
		reflect_info.push_back(createShaderReflect(vertCode));
		auto vertModule = createShaderModule(vertCode);
		stageInfo.push_back({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertModule,
		});
	}

	if (geom != nullptr) {
		auto geomCode = loadShaderCode(geom);
		reflect_info.push_back(createShaderReflect(geomCode));
		auto geomModule = createShaderModule(geomCode);
		stageInfo.push_back({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_GEOMETRY_BIT,
			.module = geomModule,
		});
	}

	{
		auto fragCode = loadShaderCode(frag);
		reflect_info.push_back(createShaderReflect(fragCode));
		auto fragModule = createShaderModule(fragCode);
		stageInfo.push_back({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragModule,
		});
	}

	std::vector<VkVertexInputAttributeDescription> vertex_attributes;
	std::vector<VkVertexInputBindingDescription>   vertex_bindings;
	VkPipelineVertexInputStateCreateInfo vertex_info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = nullptr,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = nullptr,
	};

	{ // populate vertex info
		auto& vert_reflect = reflect_info.front(); // vertex shader module
		
		for (uint32_t i = 0; i < vert_reflect.input_variable_count; ++i) {
			auto& input_info = vert_reflect.input_variables[i];

			if (input_info->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) continue;

			if (std::strcmp(input_info->name, "position")) {
				vertex_attributes.push_back({ 
					.location = input_info->location,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = input_info->word_offset.location,
				});
				++vertex_info.vertexAttributeDescriptionCount;
			} 
			else if (std::strcmp(input_info->name, "normal")) {
				vertex_attributes.push_back({ 
					.location = input_info->location,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = input_info->word_offset.location,
				});
				++vertex_info.vertexAttributeDescriptionCount;
			}
			else if (std::strcmp(input_info->name, "tangent")) {
				vertex_attributes.push_back({ 
					.location = input_info->location,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = input_info->word_offset.location,
				});
				++vertex_info.vertexAttributeDescriptionCount;
			}
			else if (std::strcmp(input_info->name, "texcoord")) {
				vertex_attributes.push_back({ 
					.location = input_info->location,
					.binding = 0,
					.format = VK_FORMAT_R16G16_SFLOAT,
					.offset = input_info->word_offset.location,
				});
				++vertex_info.vertexAttributeDescriptionCount;
			}
			else if (std::strcmp(input_info->name, "blend_id")) {
				vertex_attributes.push_back({ 
					.location = input_info->location,
					.binding = 0,
					.format = VK_FORMAT_R8G8B8A8_UINT,
					.offset = input_info->word_offset.location,
				});
				++vertex_info.vertexAttributeDescriptionCount;
			}
			else if (std::strcmp(input_info->name, "blend_weight")) {
				vertex_attributes.push_back({ 
					.location = input_info->location,
					.binding = 0,
					.format = VK_FORMAT_R8G8B8A8_SNORM,
					.offset = input_info->word_offset.location,
				});
				++vertex_info.vertexAttributeDescriptionCount;
			}
			else {
				throw "could not identify vertex attribute";
			}
		}

		vertex_info.pVertexAttributeDescriptions = vertex_attributes.data();
		vertex_info.pVertexBindingDescriptions = vertex_bindings.data();
	}
	
	VkPipelineInputAssemblyStateCreateInfo input_info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};
	
	VkPipelineTessellationStateCreateInfo tessellation_info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		// .patchControlPoints = ,
	};	

	VkPipelineViewportStateCreateInfo viewport_info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.viewportCount = 0,
		.pViewports = nullptr,
		.scissorCount = 0,
		.pScissors = nullptr,
	};

	VkPipelineRasterizationStateCreateInfo raster_info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		// .depthBiasEnable = ,
		// .depthBiasConstantFactor = ,
		// .depthBiasClamp = ,
		// .depthBiasSlopeFactor = ,
		// .lineWidth = ,
	};

	VkPipelineMultisampleStateCreateInfo multisample_info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		// .minSampleShading = ,
		// .pSampleMask = ,
		// .alphaToCoverageEnable = ,
		// .alphaToOneEnable = ,
		// .sampleShadingEnable = ,
	};

	VkPipelineDepthStencilStateCreateInfo depth_info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		// .depthTestEnable = ,
		// .depthWriteEnable = ,
		// .depthCompareOp = ,
		// .depthBoundsTestEnable = ,
		// .stencilTestEnable = ,
		// .front = ,
		// .back = ,
		// .minDepthBounds = ,
		// .maxDepthBounds = ,
	};

	VkPipelineColorBlendStateCreateInfo blend_info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		// .logicOpEnable = , 
		// .logicOp = , 
		// .attachmentCount = , 
		// .pAttachments = , 
		// .blendConstants = ,
	};


	std::vector<VkDynamicState> dynamic_states { VK_DYNAMIC_STATE_VIEWPORT, };
	VkPipelineDynamicStateCreateInfo dynamic_info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.dynamicStateCount = 0,
		.pDynamicStates = nullptr,
	};

	VkGraphicsPipelineCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = static_cast<uint32_t>(stageInfo.size()),
		.pStages = stageInfo.data(),
		.pVertexInputState = &vertex_info,
		.pInputAssemblyState = &input_info,
		.pTessellationState = nullptr,
		.pViewportState = &viewport_info,
		.pRasterizationState = &raster_info,
		.pMultisampleState = &multisample_info,
		.pDepthStencilState = &depth_info,
		.pColorBlendState = &blend_info,
		.pDynamicState = &dynamic_info,
		.layout = layout,
		// .renderPass = ,
		// .subpass = ,
		// .basePipelineHandle = ,
		// .basePipelineIndex = ,
	};

	
	vkCreateGraphicsPipelines(engine.device, nullptr, 1, &info, nullptr, &pipeline);

	for (auto& reflect : reflect_info) {
		spvReflectDestroyShaderModule(&reflect);
	}
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
*/