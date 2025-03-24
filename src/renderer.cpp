#define ARAWN_IMPLEMENTATION
#include "renderer.h"
#include "engine.h"
#include "swapchain.h"
#include "model.h"
#include "vertex.h"
#include <fstream>
#include <numeric>
bool z_prepass_enabled = false; // depth != NONE
bool deferred_enabled = false;  // render_mode == DEFERRED
bool culling_enabled = false;   // cull_mode != NONE
VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
bool msaa_enabled = sample_count == VK_SAMPLE_COUNT_1_BIT;


VkShaderModule create_shader_module(std::filesystem::path fp) {
    std::ifstream file(fp, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("could not open file: " + std::string(fp));

    std::vector<char> buffer(static_cast<std::size_t>(file.tellg()));
    file.seekg(0);
    file.read(buffer.data(), buffer.size());
    file.close();

    VkShaderModuleCreateInfo info {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.pCode = reinterpret_cast<uint32_t*>(buffer.data());
    info.codeSize = buffer.size();

    VkShaderModule shader;
    VK_ASSERT(vkCreateShaderModule(engine.device, &info, NULL, &shader));
    return shader;
}

Renderer::Renderer() {
    { // init textures
        { // init msaa attachment
            { // image
                VkImageCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.imageType = VK_IMAGE_TYPE_2D;
                info.format = swapchain.format; // !!!
                info.extent = { swapchain.extent.x, swapchain.extent.y, 1 };
                info.mipLevels = 1;
                info.arrayLayers = 1;
                info.samples = sample_count; // !!!
                info.tiling = VK_IMAGE_TILING_OPTIMAL;
                info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // !!!
                info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // !!!
                info.pQueueFamilyIndices = nullptr;
                info.queueFamilyIndexCount = 0;

                for (uint32_t i = 0; i < swapchain.frame_count; ++i)
                    VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &msaa_attachment.image[i]));
            }

            { // memory
                VkMemoryRequirements requirements;
                vkGetImageMemoryRequirements(engine.device, msaa_attachment.image[0], &requirements);

                VkMemoryAllocateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                info.pNext = nullptr;
                info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                info.allocationSize = requirements.size;
                
                for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                    VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &msaa_attachment.memory[i]));
                    VK_ASSERT(vkBindImageMemory(engine.device, msaa_attachment.image[i], msaa_attachment.memory[i], 0));
                }
            }

            { // view
                VkImageViewCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.format = swapchain.format; // !!!
                info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // !!!
                info.subresourceRange.baseMipLevel = 0;
                info.subresourceRange.levelCount = 1;
                info.subresourceRange.baseArrayLayer = 0;
                info.subresourceRange.layerCount = 1;

                for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                    info.image = msaa_attachment.image[i];
                    VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &msaa_attachment.view[i]));
                }
            }
        }

        { // depth_attachment
            { // image
                std::array<uint32_t, 2> queue_set = { engine.compute.family, engine.graphics.family };
                bool shared_memory = queue_set[0] == queue_set[1];
            
                VkImageCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.imageType = VK_IMAGE_TYPE_2D;
                info.format = VK_FORMAT_D32_SFLOAT;
                info.extent = { swapchain.extent.x, swapchain.extent.y, 1 };
                info.mipLevels = 1;
                info.arrayLayers = 1;
                info.samples = VK_SAMPLE_COUNT_1_BIT;
                info.tiling = VK_IMAGE_TILING_OPTIMAL;
                info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                info.sharingMode = shared_memory ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
                info.pQueueFamilyIndices = queue_set.data();
                info.queueFamilyIndexCount = shared_memory ? 0 : 2;

                for (uint32_t i = 0; i < swapchain.frame_count; ++i)
                    VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &depth_attachment.image[i]));
            }

            { // memory
                VkMemoryRequirements requirements;
                vkGetImageMemoryRequirements(engine.device, depth_attachment.image[0], &requirements);

                VkMemoryAllocateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                info.pNext = nullptr;
                info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                info.allocationSize = requirements.size;
                
                for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                    VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &depth_attachment.memory[i]));
                    VK_ASSERT(vkBindImageMemory(engine.device, depth_attachment.image[i], depth_attachment.memory[i], 0));
                }
            }

            { // view
                VkImageViewCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.format = VK_FORMAT_D32_SFLOAT; // !!!
                info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // !!!
                info.subresourceRange.baseMipLevel = 0;
                info.subresourceRange.levelCount = 1;
                info.subresourceRange.baseArrayLayer = 0;
                info.subresourceRange.layerCount = 1;

                for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                    info.image = depth_attachment.image[i];
                    VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &depth_attachment.view[i]));
                }
            }
        }

        if (deferred_enabled) { // albedo attachment
            { // image
                VkImageCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.imageType = VK_IMAGE_TYPE_2D;
                info.format = VK_FORMAT_R8G8B8A8_UNORM; // !!!
                info.extent = { swapchain.extent.x, swapchain.extent.y, 1 };
                info.mipLevels = 1;
                info.arrayLayers = 1;
                info.samples = VK_SAMPLE_COUNT_1_BIT; // !!!
                info.tiling = VK_IMAGE_TILING_OPTIMAL;
                info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // !!!
                info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // !!!
                info.pQueueFamilyIndices = nullptr;
                info.queueFamilyIndexCount = 0;

                for (uint32_t i = 0; i < swapchain.frame_count; ++i)
                    VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &albedo_attachment.image[i]));
            }

            { // memory
                VkMemoryRequirements requirements;
                vkGetImageMemoryRequirements(engine.device, albedo_attachment.image[0], &requirements);

                VkMemoryAllocateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                info.pNext = nullptr;
                info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                info.allocationSize = requirements.size;
                
                for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                    VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &albedo_attachment.memory[i]));
                    VK_ASSERT(vkBindImageMemory(engine.device, albedo_attachment.image[i], albedo_attachment.memory[i], 0));
                }
            }

            { // view
                VkImageViewCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.format = VK_FORMAT_R8G8B8A8_UNORM; // !!!
                info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // !!!
                info.subresourceRange.baseMipLevel = 0;
                info.subresourceRange.levelCount = 1;
                info.subresourceRange.baseArrayLayer = 0;
                info.subresourceRange.layerCount = 1;

                for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                    info.image = albedo_attachment.image[i];
                    VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &albedo_attachment.view[i]));
                }
            }
        }

        if (deferred_enabled) { // normal attachment
            { // image
                VkImageCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.imageType = VK_IMAGE_TYPE_2D;
                info.format = VK_FORMAT_R32G32B32_SFLOAT; // !!!
                info.extent = { swapchain.extent.x, swapchain.extent.y, 1 };
                info.mipLevels = 1;
                info.arrayLayers = 1;
                info.samples = VK_SAMPLE_COUNT_1_BIT; // !!!
                info.tiling = VK_IMAGE_TILING_OPTIMAL;
                info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // !!!
                info.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // !!!
                info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // !!!
                info.pQueueFamilyIndices = nullptr;
                info.queueFamilyIndexCount = 0;

                for (uint32_t i = 0; i < swapchain.frame_count; ++i)
                    VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &normal_attachment.image[i]));
            }

            { // memory
                VkMemoryRequirements requirements;
                vkGetImageMemoryRequirements(engine.device, normal_attachment.image[0], &requirements);

                VkMemoryAllocateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                info.pNext = nullptr;
                info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                info.allocationSize = requirements.size;
                
                for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                    VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &normal_attachment.memory[i]));
                    VK_ASSERT(vkBindImageMemory(engine.device, normal_attachment.image[i], normal_attachment.memory[i], 0));
                }
            }

            { // view
                VkImageViewCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.format = VK_FORMAT_R32G32B32_SFLOAT; // !!!
                info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // !!!
                info.subresourceRange.baseMipLevel = 0;
                info.subresourceRange.levelCount = 1;
                info.subresourceRange.baseArrayLayer = 0;
                info.subresourceRange.layerCount = 1;

                for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                    info.image = normal_attachment.image[i];
                    VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &normal_attachment.view[i]));
                }
            }
        }

        if (deferred_enabled) { // specular attachment
            { // image
                VkImageCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.imageType = VK_IMAGE_TYPE_2D;
                info.format = VK_FORMAT_R8G8B8A8_UNORM; // !!!
                info.extent = { swapchain.extent.x, swapchain.extent.y, 1 };
                info.mipLevels = 1;
                info.arrayLayers = 1;
                info.samples = VK_SAMPLE_COUNT_1_BIT; // !!!
                info.tiling = VK_IMAGE_TILING_OPTIMAL;
                info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // !!!
                info.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // !!!
                info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // !!!
                info.pQueueFamilyIndices = nullptr;
                info.queueFamilyIndexCount = 0;

                for (uint32_t i = 0; i < swapchain.frame_count; ++i)
                    VK_ASSERT(vkCreateImage(engine.device, &info, nullptr, &specular_attachment.image[i]));
            }

            { // memory
                VkMemoryRequirements requirements;
                vkGetImageMemoryRequirements(engine.device, specular_attachment.image[0], &requirements);

                VkMemoryAllocateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                info.pNext = nullptr;
                info.memoryTypeIndex = engine.memory_type_index(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                info.allocationSize = requirements.size;
                
                for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                    VK_ASSERT(vkAllocateMemory(engine.device, &info, nullptr, &specular_attachment.memory[i]));
                    VK_ASSERT(vkBindImageMemory(engine.device, specular_attachment.image[i], specular_attachment.memory[i], 0));
                }
            }

            { // view
                VkImageViewCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.format = VK_FORMAT_R8G8B8A8_UNORM; // !!!
                info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // !!!
                info.subresourceRange.baseMipLevel = 0;
                info.subresourceRange.levelCount = 1;
                info.subresourceRange.baseArrayLayer = 0;
                info.subresourceRange.layerCount = 1;

                for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                    info.image = specular_attachment.image[i];
                    VK_ASSERT(vkCreateImageView(engine.device, &info, nullptr, &specular_attachment.view[i]));
                }
            }
        }
    }

    if (z_prepass_enabled) { // depth pass
        { // initialize cmd buffers
            VkCommandBufferAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            info.pNext = nullptr;
            info.commandPool = engine.graphics.pool;
            info.commandBufferCount = swapchain.frame_count;
            info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &info, &depth_pass.cmd_buffer[0]));    
        }

        { // initialize semaphores
            VkSemaphoreCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            info.flags = 0;
            info.pNext = nullptr;

            for (uint32_t i = 0; i < swapchain.frame_count; ++i)
                VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &depth_pass.finished[i]));
        }
        
        { // create pipeline layout
            std::array<VkDescriptorSetLayout, 2> sets = { engine.object_layout, engine.camera_layout };

            VkPipelineLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.pushConstantRangeCount = 0;
            info.pPushConstantRanges = nullptr;
            info.setLayoutCount = sets.size();
            info.pSetLayouts = sets.data();

            VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &depth_pass.layout));
        }

        { // create renderpass
            std::array<VkAttachmentDescription, 1> attachments;
            attachments[0].flags = 0;
            attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments[0].format = VK_FORMAT_D32_SFLOAT;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            VkAttachmentReference depth_ref;
            depth_ref.attachment = 0;
            depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            std::array<VkSubpassDescription, 1> subpasses;
            subpasses[0].flags = 0;
            subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpasses[0].pPreserveAttachments = nullptr;
            subpasses[0].preserveAttachmentCount = 0;
            subpasses[0].pInputAttachments = nullptr;
            subpasses[0].inputAttachmentCount = 0;
            subpasses[0].pColorAttachments = nullptr;
            subpasses[0].colorAttachmentCount = 0;
            subpasses[0].pResolveAttachments = nullptr;
            subpasses[0].pDepthStencilAttachment = &depth_ref;
            
            VkRenderPassCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.attachmentCount = attachments.size();
            info.pAttachments = attachments.data();
            info.subpassCount = subpasses.size();
            info.pSubpasses = subpasses.data();
            info.dependencyCount = 0;
            info.pDependencies = nullptr;

            VK_ASSERT(vkCreateRenderPass(engine.device, &info, nullptr, &depth_pass.renderpass));
        }
        
        { // create pipeline
            VkShaderModule vert_module = create_shader_module("res/import/shaders/depth.vert.spv");
            VkShaderModule frag_module = create_shader_module("res/import/shaders/depth.frag.spv");

            std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
            shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stages[0].pNext = nullptr;
            shader_stages[0].flags = 0;
            shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shader_stages[0].module = vert_module;
            shader_stages[0].pName = "main";
            shader_stages[0].pSpecializationInfo = nullptr;

            shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stages[1].pNext = nullptr;
            shader_stages[1].flags = 0;
            shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shader_stages[1].module = frag_module;
            shader_stages[1].pName = "main";
            shader_stages[1].pSpecializationInfo = nullptr;

            std::array<VkVertexInputBindingDescription, 1> vertex_binding;
            vertex_binding[0].binding = 0;
            vertex_binding[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            vertex_binding[0].stride = sizeof(Vertex);

            std::array<VkVertexInputAttributeDescription, 1> vertex_attrib; // max vertex count
            vertex_attrib[0].binding = 0;
            vertex_attrib[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_attrib[0].location = 0;
            vertex_attrib[0].offset = offsetof(Vertex, position);

            VkPipelineVertexInputStateCreateInfo vertex_input{};
            vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input.vertexBindingDescriptionCount = vertex_binding.size();
            vertex_input.pVertexBindingDescriptions = vertex_binding.data();
            vertex_input.vertexAttributeDescriptionCount = vertex_attrib.size();
            vertex_input.pVertexAttributeDescriptions = vertex_attrib.data();

            VkPipelineInputAssemblyStateCreateInfo assembly{};
            assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            assembly.primitiveRestartEnable = VK_FALSE;

            VkPipelineViewportStateCreateInfo viewport_state{};
            viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state.viewportCount = 1;
            viewport_state.scissorCount = 1;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // TODO: wireframe debug mode
            rasterizer.lineWidth = 1.0f; // only used when VK_POLYGON_MODE_LINE enabled
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // COUNTER_
            rasterizer.depthBiasEnable = VK_FALSE;

            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            VkPipelineDepthStencilStateCreateInfo depth_stencil{};
            depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil.depthTestEnable = VK_TRUE;
            depth_stencil.depthWriteEnable = VK_TRUE;
            depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depth_stencil.depthBoundsTestEnable = VK_FALSE;
            depth_stencil.stencilTestEnable = VK_FALSE;

            VkPipelineColorBlendAttachmentState resolve_attachment{};
            resolve_attachment.colorWriteMask = 0;
            resolve_attachment.blendEnable = VK_FALSE;
            
            VkPipelineColorBlendStateCreateInfo colour_blending{};
            colour_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colour_blending.logicOpEnable = VK_FALSE;
            colour_blending.logicOp = VK_LOGIC_OP_COPY;
            colour_blending.attachmentCount = 1;
            colour_blending.pAttachments = &resolve_attachment;
            colour_blending.blendConstants[0] = 0.0f;
            colour_blending.blendConstants[1] = 0.0f;
            colour_blending.blendConstants[2] = 0.0f;
            colour_blending.blendConstants[3] = 0.0f;

            VkDynamicState dynamic_states[] { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            
            VkPipelineDynamicStateCreateInfo dynamics{};
            dynamics.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamics.dynamicStateCount = 2;
            dynamics.pDynamicStates = dynamic_states;

            VkGraphicsPipelineCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            info.stageCount = 2;
            info.pStages = shader_stages.data();
            info.pVertexInputState = &vertex_input;
            info.pInputAssemblyState = &assembly;
            info.pViewportState = &viewport_state;
            info.pRasterizationState = &rasterizer;
            info.pMultisampleState = &multisampling;
            info.pDepthStencilState = &depth_stencil;
            info.pColorBlendState = &colour_blending;
            info.pDynamicState = &dynamics;
            info.layout = depth_pass.layout;
            info.renderPass = depth_pass.renderpass;
            info.subpass = 0;
            info.basePipelineHandle = VK_NULL_HANDLE;
            VK_ASSERT(vkCreateGraphicsPipelines(engine.device, nullptr, 1, &info, nullptr, &depth_pass.pipeline));

            vkDestroyShaderModule(engine.device, vert_module, nullptr);
            vkDestroyShaderModule(engine.device, frag_module, nullptr);
        }

        { // initialize framebuffers
            std::array<VkImageView, 1> attachments;

            VkFramebufferCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.layers = 1;
            info.renderPass = depth_pass.renderpass;
            info.width = swapchain.extent.x;
            info.height = swapchain.extent.y;
            info.pAttachments = attachments.data();
            info.attachmentCount = attachments.size(); 
            
            for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                attachments[i] = depth_attachment.view[i];
                VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &depth_pass.framebuffer[i]));
            }
        }
    }

    if (culling_enabled) { // TODO: cluster pass
        { // initialize cmd_buffers

        }

        { // initialize semaphores

        }

        { // initialize layouts

        }

        { // initialize pipeline

        }
    }

    if (deferred_enabled) { // TODO: deferred pass
        { // initialize cmd_buffers

        }

        { // initialize semaphores

        }

        { // initialize fences

        }

        { // initialize layouts

        }

        { // initialize renderpass
            
        }

        { // initialize pipeline

        }

        { // framebuffer

        }
    }

    { // present pass
        { // initialize cmd buffers
            VkCommandBufferAllocateInfo info{ };
            info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            info.pNext = nullptr;
            info.commandPool = engine.graphics.pool;
            info.commandBufferCount = swapchain.frame_count;
            info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

            VK_ASSERT(vkAllocateCommandBuffers(engine.device, &info, lighting_pass.cmd_buffer.data()));
        }

        { // initialize semaphores
            VkSemaphoreCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            info.flags = 0;
            info.pNext = nullptr;
            
            for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
                VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &lighting_pass.finished[i]));
                VK_ASSERT(vkCreateSemaphore(engine.device, &info, nullptr, &lighting_pass.image_available[i]));
            }
        }

        { // initialize fences
            VkFenceCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            for (uint32_t i = 0; i < swapchain.frame_count; ++i)
                vkCreateFence(engine.device, &info, nullptr, &lighting_pass.in_flight[i]);
        }

        { // initialize pipeline layout
            std::array<VkDescriptorSetLayout, 3> sets{ engine.object_layout, engine.camera_layout, engine.material_layout };
            std::array<VkPushConstantRange, 1> ranges;

            ranges[0].size = sizeof(PushConstants);
            ranges[0].offset = 0;
            ranges[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkPipelineLayoutCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.pushConstantRangeCount = ranges.size();
            info.pPushConstantRanges = ranges.data();
            info.setLayoutCount = sets.size();
            info.pSetLayouts = sets.data();

            VK_ASSERT(vkCreatePipelineLayout(engine.device, &info, nullptr, &lighting_pass.layout));
        }

        uint32_t attachment_count = 0;
        { // initialize renderpass
            VkAttachmentDescription attachment_data[6]; // max 6: swapchain, msaa, depth, albedo, normal, specular  
            std::array<VkAttachmentReference, 3>  input_ref;
            std::array<VkAttachmentReference, 1>  colour_ref;
            VkAttachmentReference depth_ref, resolve_ref;

            // bindings
            if (msaa_enabled) { // init swapchain image and msaa staging attachment attachment                 
                /* 
                when msaa enabled, the renderpass writes first to the colour attachment and then resolves the 
                multi sampling into the resolve attachment. I want to write to the colour attachment and 
                resolve onto the screen. swapchain image always bound to attachment 0.
                */
                
                resolve_ref.attachment = attachment_count++;
                resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription& swapchain_info = attachment_data[resolve_ref.attachment];
                swapchain_info.flags = 0;
                swapchain_info.format = swapchain.format;
                swapchain_info.samples = VK_SAMPLE_COUNT_1_BIT;
                swapchain_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                swapchain_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                swapchain_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                swapchain_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                swapchain_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                swapchain_info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                colour_ref[0].attachment = attachment_count++;
                colour_ref[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription& msaa_staging_info = attachment_data[colour_ref[0].attachment];
                msaa_staging_info.flags = 0;
                msaa_staging_info.format = swapchain.format;
                msaa_staging_info.samples = sample_count;
                msaa_staging_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                msaa_staging_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                msaa_staging_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                msaa_staging_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                msaa_staging_info.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                msaa_staging_info.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;                
            } else {
                colour_ref[0].attachment = attachment_count++;
                colour_ref[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription& swapchain_info = attachment_data[colour_ref[0].attachment];
                swapchain_info.flags = 0;
                swapchain_info.format = swapchain.format;
                swapchain_info.samples = VK_SAMPLE_COUNT_1_BIT;
                swapchain_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                swapchain_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                swapchain_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                swapchain_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                swapchain_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                swapchain_info.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            }

            if (z_prepass_enabled) { // init depth attachment(read only)
                depth_ref.attachment = attachment_count++;
                depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription& depth_info = attachment_data[depth_ref.attachment];
                depth_info.flags = 0;
                depth_info.format = VK_FORMAT_D32_SFLOAT;
                depth_info.samples = VK_SAMPLE_COUNT_1_BIT;
                depth_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                depth_info.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depth_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depth_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depth_info.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depth_info.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


            }
            else { // init depth attachment(read write)
                depth_ref.attachment = attachment_count++;
                depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription& depth_info = attachment_data[depth_ref.attachment];
                depth_info.flags = 0;
                depth_info.format = VK_FORMAT_D32_SFLOAT;
                depth_info.samples = VK_SAMPLE_COUNT_1_BIT;
                depth_info.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depth_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depth_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depth_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depth_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depth_info.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }   

            if (deferred_enabled) { // init albedo, normal, specular attachment(g-buffer)
                input_ref[0].attachment = attachment_count++;
                VkAttachmentDescription& albedo_info = attachment_data[input_ref[0].attachment];
                
                albedo_info.flags = 0;
                albedo_info.format = swapchain.format;
                albedo_info.samples = VK_SAMPLE_COUNT_1_BIT;
                albedo_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                albedo_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                albedo_info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                albedo_info.storeOp = VK_ATTACHMENT_STORE_OP_NONE;
                albedo_info.initialLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                albedo_info.finalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                

                input_ref[1].attachment = attachment_count++;
                VkAttachmentDescription& normal_info = attachment_data[input_ref[1].attachment];
                
                normal_info.flags = 0;
                normal_info.format = VK_FORMAT_R32G32B32_SFLOAT;
                normal_info.samples = VK_SAMPLE_COUNT_1_BIT;
                normal_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                normal_info.storeOp = VK_ATTACHMENT_STORE_OP_NONE;
                normal_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                normal_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                normal_info.initialLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                normal_info.finalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;

                input_ref[2].attachment = attachment_count++;
                VkAttachmentDescription& specular_info = attachment_data[input_ref[1].attachment];
                
                specular_info.flags = 0;
                specular_info.format = VK_FORMAT_R8G8B8A8_UNORM;
                specular_info.samples = VK_SAMPLE_COUNT_1_BIT;
                specular_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                specular_info.storeOp = VK_ATTACHMENT_STORE_OP_NONE;
                specular_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                specular_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                specular_info.initialLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                specular_info.finalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;            
            }

            std::span<VkAttachmentDescription> attachments{ attachment_data, attachment_count };

            std::array<VkSubpassDescription, 1> subpass;
            subpass[0].flags = 0;
            subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass[0].pPreserveAttachments = nullptr;
            subpass[0].preserveAttachmentCount = 0;
            subpass[0].inputAttachmentCount = deferred_enabled ? input_ref.size() : 0;
            subpass[0].pInputAttachments = input_ref.data();
            subpass[0].colorAttachmentCount = colour_ref.size();
            subpass[0].pColorAttachments = colour_ref.data();
            subpass[0].pResolveAttachments = msaa_enabled ? &resolve_ref : nullptr;
            subpass[0].pDepthStencilAttachment = &depth_ref;

            VkRenderPassCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.attachmentCount = attachments.size();
            info.pAttachments = attachments.data();
            info.subpassCount = subpass.size();
            info.pSubpasses = subpass.data();
            info.dependencyCount = 0;
            info.pDependencies = nullptr;

            VK_ASSERT(vkCreateRenderPass(engine.device, &info, nullptr, &lighting_pass.renderpass));
        }

        { // initialize pipeline
            VkShaderModule vert_module;
            VkShaderModule frag_module;
            if (deferred_enabled) {
                vert_module = create_shader_module("res/import/shaders/present.vert.spv");
                frag_module = create_shader_module("res/import/shaders/present.frag.spv");
            } else {
                vert_module = create_shader_module("res/import/shaders/forward.vert.spv");
                frag_module = create_shader_module("res/import/shaders/forward.frag.spv");
            }

            VkPipelineShaderStageCreateInfo shader_stages[2];
            shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stages[0].pNext = nullptr;
            shader_stages[0].flags = 0;
            shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shader_stages[0].module = vert_module;
            shader_stages[0].pName = "main";
            shader_stages[0].pSpecializationInfo = nullptr;

            shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stages[1].pNext = nullptr;
            shader_stages[1].flags = 0;
            shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shader_stages[1].module = frag_module;
            shader_stages[1].pName = "main";
            shader_stages[1].pSpecializationInfo = nullptr;

            uint32_t vertex_attrib_count = 0, vertex_binding_count = 0;
            std::array<VkVertexInputBindingDescription, 1> vertex_binding;
            std::array<VkVertexInputAttributeDescription, 4> vertex_attrib;
            
            if (!deferred_enabled) {
                vertex_binding[0].binding = 0;
                vertex_binding[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                vertex_binding[0].stride = sizeof(Vertex);
                ++vertex_binding_count;

                vertex_attrib[vertex_attrib_count].binding = 0;
                vertex_attrib[vertex_attrib_count].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertex_attrib[vertex_attrib_count].location = 0;
                vertex_attrib[vertex_attrib_count].offset = offsetof(Vertex, position);
                ++vertex_attrib_count;

                vertex_attrib[vertex_attrib_count].binding = 0;
                vertex_attrib[vertex_attrib_count].format = VK_FORMAT_R32G32_SFLOAT;
                vertex_attrib[vertex_attrib_count].location = 1;
                vertex_attrib[vertex_attrib_count].offset = offsetof(Vertex, texcoord);
                ++vertex_attrib_count;

                vertex_attrib[vertex_attrib_count].binding = 0;
                vertex_attrib[vertex_attrib_count].format = VK_FORMAT_R32G32B32_SFLOAT;
                vertex_attrib[vertex_attrib_count].location = 2;
                vertex_attrib[vertex_attrib_count].offset = offsetof(Vertex, normal);
                ++vertex_attrib_count;

                vertex_attrib[vertex_attrib_count].binding = 0;
                vertex_attrib[vertex_attrib_count].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                vertex_attrib[vertex_attrib_count].location = 3;
                vertex_attrib[vertex_attrib_count].offset = offsetof(Vertex, tangent);
                ++vertex_attrib_count;
            }
            
            VkPipelineVertexInputStateCreateInfo vertex_input{};
            vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input.vertexBindingDescriptionCount = vertex_binding_count;
            vertex_input.pVertexBindingDescriptions = vertex_binding.data();
            vertex_input.vertexAttributeDescriptionCount = vertex_attrib_count;
            vertex_input.pVertexAttributeDescriptions = vertex_attrib.data();

            VkPipelineInputAssemblyStateCreateInfo assembly{};
            assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            assembly.primitiveRestartEnable = VK_FALSE;

            VkPipelineViewportStateCreateInfo viewport_state{};
            viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state.viewportCount = 1;
            viewport_state.scissorCount = 1;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // TODO: wireframe debug mode
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // COUNTER_
            rasterizer.depthBiasEnable = VK_FALSE;

            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE; // ? I dont understand what this does at all
            multisampling.rasterizationSamples = sample_count;

            VkPipelineDepthStencilStateCreateInfo depth_stencil{};
            depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depth_stencil.depthTestEnable = VK_TRUE;
            depth_stencil.depthWriteEnable = VK_TRUE;
            depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depth_stencil.depthBoundsTestEnable = VK_FALSE;
            depth_stencil.stencilTestEnable = VK_FALSE;

            VkPipelineColorBlendAttachmentState resolve_attachment{};
            resolve_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            resolve_attachment.blendEnable = VK_FALSE;
            
            VkPipelineColorBlendStateCreateInfo colour_blending{};
            colour_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colour_blending.logicOpEnable = VK_FALSE;
            colour_blending.logicOp = VK_LOGIC_OP_COPY;
            colour_blending.attachmentCount = 1;
            colour_blending.pAttachments = &resolve_attachment;
            colour_blending.blendConstants[0] = 0.0f;
            colour_blending.blendConstants[1] = 0.0f;
            colour_blending.blendConstants[2] = 0.0f;
            colour_blending.blendConstants[3] = 0.0f;

            VkDynamicState dynamic_states[] { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            
            VkPipelineDynamicStateCreateInfo dynamics{};
            dynamics.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamics.dynamicStateCount = 2;
            dynamics.pDynamicStates = dynamic_states;

            VkGraphicsPipelineCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            info.stageCount = 2;
            info.pStages = shader_stages;
            info.pVertexInputState = &vertex_input;
            info.pInputAssemblyState = &assembly;
            info.pViewportState = &viewport_state;
            info.pRasterizationState = &rasterizer;
            info.pMultisampleState = &multisampling;
            info.pDepthStencilState = &depth_stencil;
            info.pColorBlendState = &colour_blending;
            info.pDynamicState = &dynamics;
            info.layout = lighting_pass.layout;
            info.renderPass = lighting_pass.renderpass;
            info.subpass = 0;
            info.basePipelineHandle = VK_NULL_HANDLE;
            VK_ASSERT(vkCreateGraphicsPipelines(engine.device, nullptr, 1, &info, nullptr, &lighting_pass.pipeline));

            vkDestroyShaderModule(engine.device, vert_module, nullptr);
            vkDestroyShaderModule(engine.device, frag_module, nullptr);
        }

        { // initialize framebuffers
            VkImageView attachment_data[6];
            std::span<VkImageView> attachments { attachment_data, attachment_count };

            VkFramebufferCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.layers = 1;
            info.renderPass = lighting_pass.renderpass;
            info.width = swapchain.extent.x;
            info.height = swapchain.extent.y;                
            info.pAttachments = attachments.data();
            info.attachmentCount = attachments.size();

            lighting_pass.framebuffer.resize(std::lcm(swapchain.frame_count, swapchain.image_count));

            for (uint32_t i = 0; i < lighting_pass.framebuffer.size(); ++i) {

                uint32_t frame_i = i % swapchain.frame_count;
                uint32_t image_i = i % swapchain.image_count;
                
                uint32_t attachment_index = 0;
                attachments[attachment_index] = swapchain.views[image_i];
                
                if (msaa_enabled) {
                    attachments[++attachment_index] = msaa_attachment.view[frame_i];
                }

                attachments[++attachment_index] = depth_attachment.view[frame_i];
                
                if (deferred_enabled) {
                    attachments[++attachment_index] = albedo_attachment.view[frame_i];
                    attachments[++attachment_index] = normal_attachment.view[frame_i];
                    attachments[++attachment_index] = specular_attachment.view[frame_i];
                }

                VK_ASSERT(vkCreateFramebuffer(engine.device, &info, nullptr, &lighting_pass.framebuffer[i]));
            }
        }
    }
}

Renderer::~Renderer() {
    vkDeviceWaitIdle(engine.device);

    if (z_prepass_enabled) { // destroy depth pass
        for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
            vkDestroyFramebuffer(engine.device, depth_pass.framebuffer[i], nullptr);
        }
    }

    if (deferred_enabled) { // destroy deferred pass
        for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
            vkDestroyFramebuffer(engine.device, deferred_pass.framebuffer[i], nullptr);
        }
    }

    if (culling_enabled) { // destroy culling pass

    }

    { // destroy present pass
        for (auto framebuffer : lighting_pass.framebuffer) {
            vkDestroyFramebuffer(engine.device, framebuffer, nullptr);
        }

        for (uint32_t i = 0; i < swapchain.frame_count; ++i) {
            vkDestroySemaphore(engine.device, lighting_pass.image_available[i], nullptr);
            vkDestroyFence(engine.device, lighting_pass.in_flight[i], nullptr);
        }
    }
}

void Renderer::draw() {
    const uint64_t timeout = 1000000000;
    
    VK_ASSERT(vkWaitForFences(engine.device, 1, &lighting_pass.in_flight[frame_index], true, timeout));
    
    uint32_t image_index;
    VkResult res = vkAcquireNextImageKHR(engine.device, swapchain.swapchain, timeout, lighting_pass.image_available[frame_index], nullptr, &image_index);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        swapchain.recreate();
        // update texture
        // framebuffers
        return; 
    } else if (res != VK_SUBOPTIMAL_KHR) {
        VK_ASSERT(res);
    }
    
    // update ubos
    {

    }

    VK_ASSERT(vkResetFences(engine.device, 1, &lighting_pass.in_flight[frame_index]));
    
    if (z_prepass_enabled) {
        depth_pass.record(frame_index);
        depth_pass.submit(frame_index);
    }
    
    if (deferred_enabled) { // deferred pass 
        deferred_pass.record(frame_index);

        VkSemaphore wait_semaphore[] = { depth_pass.finished[frame_index] }; // wait on depth pass
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        deferred_pass.submit(frame_index, wait_semaphore, wait_stages, nullptr);
    }

    if (culling_enabled) { // cluster pass
        culling_pass.record(frame_index);

        VkSemaphore wait_semaphore[] = { depth_pass.finished[frame_index] }; // wait on depth pass
        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        culling_pass.submit(frame_index, wait_semaphore, wait_stages, nullptr);
    }
    
    { // lighting pass
        lighting_pass.record(frame_index);

        VkSemaphore semaphores[] = { deferred_pass.finished[frame_index], lighting_pass.image_available[frame_index], culling_pass.finished[frame_index] };
        VkPipelineStageFlags stages[] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
        
        // skip waits on deferred & culling if not enabled
        std::span<VkSemaphore> wait_semaphores = { semaphores + (!deferred_enabled), semaphores + 3 - (!culling_enabled) };
        std::span<VkPipelineStageFlags> wait_stages = { stages + (!deferred_enabled), stages + 3 - (!culling_enabled) };
        
        lighting_pass.submit(frame_index, wait_semaphores, wait_stages, lighting_pass.in_flight[frame_index]);
    }
    
    { // present to screen
        std::array<VkSemaphore, 1> wait { lighting_pass.finished[frame_index] };
        
        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.pNext = nullptr;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = wait.data();
        info.swapchainCount = wait.size();
        info.pSwapchains = &swapchain.swapchain;
        info.pImageIndices = &image_index;
        info.pResults = nullptr;
        
        VkResult res = vkQueuePresentKHR(engine.present.queue, &info);

        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            swapchain.recreate();
            // recreate:
            // - attachments
            // - framebuffers
        } else {
            VK_ASSERT(res);
        }
    }
    
    frame_index = (frame_index + 1) % swapchain.frame_count; // get next frame data set
}



Renderer::TextureAttachment::~TextureAttachment() {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT && memory[i]; ++i) {
        vkDestroyImage(engine.device, image[i], nullptr);
        vkDestroyImageView(engine.device, view[i], nullptr);
        vkFreeMemory(engine.device, memory[i], nullptr);
    }
}

Renderer::BufferAttachment::~BufferAttachment() {
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT && memory[i]; ++i) {
        vkDestroyBuffer(engine.device, buffer[i], nullptr);
        vkFreeMemory(engine.device, memory[i], nullptr);
    }
}

Renderer::GraphicPipeline::~GraphicPipeline() {
    uint32_t frame_count;
    for (frame_count = 0; frame_count < MAX_FRAMES_IN_FLIGHT && cmd_buffer[frame_count]; ++frame_count);
    if (frame_count == 0) return;

    vkDestroyPipeline(engine.device, pipeline, nullptr);
    vkDestroyRenderPass(engine.device, renderpass, nullptr);
    vkDestroyPipelineLayout(engine.device, layout, nullptr);

    vkFreeCommandBuffers(engine.device, engine.graphics.pool, frame_count, cmd_buffer.data());

    for (uint32_t i = 0; i < frame_count; ++i)
        vkDestroySemaphore(engine.device, finished[i], nullptr);
}

void Renderer::GraphicPipeline::submit(
    uint32_t frame_index, 
    std::span<VkSemaphore> wait_semaphore, 
    std::span<VkPipelineStageFlags> wait_stages, 
    VkFence signal_fence
) {
    VkSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount = wait_semaphore.size();
    info.pWaitSemaphores = wait_semaphore.data();
    info.pWaitDstStageMask = wait_stages.data();
    info.commandBufferCount = 1;
    info.pCommandBuffers = &cmd_buffer[frame_index];
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &finished[frame_index];
    
    vkQueueSubmit(engine.graphics.queue, 1, &info, signal_fence);
}

Renderer::ComputePipeline::~ComputePipeline() {
    uint32_t frame_count;
    for (frame_count = 0; frame_count < MAX_FRAMES_IN_FLIGHT && cmd_buffer[frame_count]; ++frame_count);
    if (frame_count == 0) return;

    vkDestroyPipeline(engine.device, pipeline, nullptr);
    vkDestroyPipelineLayout(engine.device, layout, nullptr);

    vkFreeCommandBuffers(engine.device, engine.compute.pool, frame_count, cmd_buffer.data());

    for (uint32_t i = 0; i < frame_count; ++i)
        vkDestroySemaphore(engine.device, finished[i], nullptr);
}

void Renderer::ComputePipeline::submit(
    uint32_t frame_index, 
    std::span<VkSemaphore> wait_semaphore, 
    std::span<VkPipelineStageFlags> wait_stages, 
    VkFence signal_fence) 
{
    VkSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount = wait_semaphore.size();
    info.pWaitSemaphores = wait_semaphore.data();
    info.pWaitDstStageMask = wait_stages.data();
    info.commandBufferCount = 1;
    info.pCommandBuffers = &cmd_buffer[frame_index];
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &finished[frame_index];
    
    vkQueueSubmit(engine.compute.queue, 1, &info, nullptr);
}

void Renderer::DepthPass::record(uint32_t frame_index) {

    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }

    { // begin renderpass
        VkClearValue clear = { 1.0f };
        
        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = renderpass;
        info.renderArea.offset = { 0, 0 };
        info.renderArea.extent = { swapchain.extent.x, swapchain.extent.y };
        info.framebuffer = framebuffer[frame_index];
        info.pClearValues = &clear;
        info.clearValueCount = 1;

        vkCmdBeginRenderPass(cmd_buffer[frame_index], &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    { // bind pipeline
        vkCmdBindPipeline(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapchain.extent.x;
        viewport.height = (float) swapchain.extent.y;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buffer[frame_index], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { swapchain.extent.x, swapchain.extent.y };
        vkCmdSetScissor(cmd_buffer[frame_index], 0, 1, &scissor);
    }

    for (Model& model : models) {
        // TODO: bind transform descriptor set

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd_buffer[frame_index], 0, 1, &model.vertex_buffer, offsets);
        
        uint32_t index_offset = 0;
        vkCmdBindIndexBuffer(cmd_buffer[frame_index], model.index_buffer, index_offset, VK_INDEX_TYPE_UINT32);

        for (uint32_t i = 0; i < model.meshes.size(); ++i) {
            vkCmdDrawIndexed(cmd_buffer[frame_index], model.meshes[i].vertex_count, 1, index_offset, 0, 0);
            index_offset += model.meshes[i].vertex_count;
        }
    }
    
    { // end renderpass & cmd buffer
        vkCmdEndRenderPass(cmd_buffer[frame_index]);

        VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
    }
}

void Renderer::ClusterPass::record(uint32_t frame_index) {
    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }



    VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
}

void Renderer::DeferredPass::record(uint32_t frame_index) {
    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }

    { // begin renderpass
        VkClearValue clear[3];
        clear[0].color = { 0.0, 0.0, 0.0, 0.0 };
        clear[1].color = { 0.0, 0.0, 0.0, 0.0 };
        clear[2].color = { 0.0, 0.0, 0.0, 0.0 };
        
        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = renderpass;
        info.renderArea.offset = { 0, 0 };
        info.renderArea.extent = { swapchain.extent.x, swapchain.extent.y };
        info.framebuffer = framebuffer[frame_index];
        info.pClearValues = clear;
        info.clearValueCount = 3;

        vkCmdBeginRenderPass(cmd_buffer[frame_index], &info, VK_SUBPASS_CONTENTS_INLINE);
    }



    vkCmdEndRenderPass(cmd_buffer[frame_index]);
    
    VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
}

void Renderer::LightingPass::record(uint32_t frame_index) {
    { // reset & begin cmd buffer
        VK_ASSERT(vkResetCommandBuffer(cmd_buffer[frame_index], 0));
        
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_ASSERT(vkBeginCommandBuffer(cmd_buffer[frame_index], &info));
    }

    { // begin renderpass
        VkClearValue clear[1];
        clear[0].color = { 0.05, 0.05, 0.05 };
        
        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = renderpass;
        info.renderArea.offset = { 0, 0 };
        info.renderArea.extent = { swapchain.extent.x, swapchain.extent.y };
        info.framebuffer = framebuffer[frame_index];
        info.pClearValues = clear;
        info.clearValueCount = 1;

        vkCmdBeginRenderPass(cmd_buffer[frame_index], &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    { // bind pipeline
        vkCmdBindPipeline(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapchain.extent.x;
        viewport.height = (float) swapchain.extent.y;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buffer[frame_index], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { swapchain.extent.x, swapchain.extent.y };
        vkCmdSetScissor(cmd_buffer[frame_index], 0, 1, &scissor);
    }

    for (Model& model : models) {
        //vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &model.transform.set, 0, nullptr);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd_buffer[frame_index], 0, 1, &model.vertex_buffer, offsets);
        
        uint32_t index_offset = 0;
        vkCmdBindIndexBuffer(cmd_buffer[frame_index], model.index_buffer, index_offset, VK_INDEX_TYPE_UINT32);

        for (uint32_t i = 0; i < model.meshes.size(); ++i) {
            vkCmdBindDescriptorSets(cmd_buffer[frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2/*material set*/, 1, &model.meshes[i].material.set, 0, nullptr);
            vkCmdDrawIndexed(cmd_buffer[frame_index], model.meshes[i].vertex_count, 1, index_offset, 0, 0);
            index_offset += model.meshes[i].vertex_count;
        }
    }
    
    { // end render pass & cmd buffer
        vkCmdEndRenderPass(cmd_buffer[frame_index]);

        VK_ASSERT(vkEndCommandBuffer(cmd_buffer[frame_index]));
    }
}