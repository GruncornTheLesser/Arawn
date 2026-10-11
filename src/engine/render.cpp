#define ARAWN_INCLUDE_VULKAN
#include <engine/render.h>
#include <common/settings.h>
#include <nodes/camera.h>
#include <nodes/light.h>
#include <algorithm>
#include <array>
#include <span>
#include <vector>

using namespace arawn;

constexpr VkFormat kColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;

constexpr uint32_t kMaxInstances     = 1u << 16;
constexpr uint32_t kMaxLights        = 1u << 10;
constexpr uint32_t kMaxLightsPerTile = 256;
constexpr uint32_t kMaxTiles         = 240 * 135;
constexpr uint32_t kMaxClusters      = 16 * 9 * 24; // ???
constexpr uint32_t kDepthMipCount    = 15;

namespace arawn::shaders {

    alignas(uint32_t) const unsigned char depth_tree_comp[] = {
    #embed "shaders/depth_tree.comp.spv"
    };
    
    alignas(uint32_t) const unsigned char depth_frag[] = {
    #embed "shaders/depth.frag.spv"
    };
    
    alignas(uint32_t) const unsigned char depth_vert[] = {
    #embed "shaders/depth.vert.spv"
    };
    
    alignas(uint32_t) const unsigned char forward_frag[] = {
    #embed "shaders/forward.frag.spv"
    };
    
    alignas(uint32_t) const unsigned char forward_vert[] = {
    #embed "shaders/forward.vert.spv"
    };
    
    alignas(uint32_t) const unsigned char frust_cull_comp[] = {
    #embed "shaders/frust_cull.comp.spv"
    };
    
    alignas(uint32_t) const unsigned char gen_clusters_comp[] = {
    #embed "shaders/gen_clusters.comp.spv"
    };
    
    alignas(uint32_t) const unsigned char gen_tiles_comp[] = {
    #embed "shaders/gen_tiles.comp.spv"
    };
    
    alignas(uint32_t) const unsigned char mesh_cull_comp[] = {
    #embed "shaders/mesh_cull.comp.spv"
    };
    
    alignas(uint32_t) const unsigned char occl_cull_comp[] = {
    #embed "shaders/occl_cull.comp.spv"
    };
    
    alignas(uint32_t) const unsigned char tile_cull_comp[] = {
    #embed "shaders/tile_cull.comp.spv"
    };
}

void engine::Render::create(const Core &core, const Window &window, const Device &device, const World& world, const Settings &settings) {
    frames.resize(settings.buffering == BufferingMode::DOUBLE ? 2 : 3);

    { // create descriptor layout
        auto bindings = std::to_array<VkDescriptorSetLayoutBinding>({
            { 
                .binding = 0, 
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         
                .descriptorCount = 1,  
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT,
                .pImmutableSamplers = nullptr
            },
            { 
                .binding = 1, 
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         
                .descriptorCount = 1,  
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            },
            { 
                .binding = 2, 
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         
                .descriptorCount = 1,  
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            },
            { 
                .binding = 3, 
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         
                .descriptorCount = 1,  
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            },
            { 
                .binding = 4, 
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         
                .descriptorCount = 1,  
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            },
            { 
                .binding = 5, 
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         
                .descriptorCount = 1,  
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr
            },
            { 
                .binding = 6, 
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         
                .descriptorCount = 1,  
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr
            },
            { 
                .binding = 7, 
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
                .descriptorCount = 1,  
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            },
            { 
                .binding = 8, 
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          
                .descriptorCount = 15, 
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr
            },
        });

        VkDescriptorSetLayoutCreateInfo setInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        };
        VK_ASSERT(vkCreateDescriptorSetLayout(device.device, &setInfo, nullptr, &descriptor.layout));
    }

    { // create descriptor pool
        const uint32_t n = static_cast<uint32_t>(frames.size());
        auto sizes = std::to_array<VkDescriptorPoolSize>({
            { 
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         
                .descriptorCount = 1 * n
            },
            { 
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         
                .descriptorCount = 6 * n
            },
            { 
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
                .descriptorCount = 1 * n
            },
            { 
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          
                .descriptorCount = kDepthMipCount * n
            },
        });
        VkDescriptorPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .maxSets = n,
            .poolSizeCount = static_cast<uint32_t>(sizes.size()),
            .pPoolSizes = sizes.data(),
        };
        VK_ASSERT(vkCreateDescriptorPool(device.device, &poolInfo, nullptr, &descriptor.pool));
    }
    
    { // create pipelines
        auto sets = std::to_array<VkDescriptorSetLayout>({
            descriptor.layout,
            world.descriptor.layout
        });
        auto createCompute = [&](
            uint32_t pushData,
            std::span<const unsigned char> compSpv,
            auto* out
        ) {
            VkPushConstantRange range{
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                .offset = 0,
                .size = pushData
            };
            VkPipelineLayoutCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = static_cast<uint32_t>(sets.size()),
                .pSetLayouts = sets.data(),
                .pushConstantRangeCount = pushData ? 1u : 0u,
                .pPushConstantRanges = pushData ? &range : nullptr,
            };
            VK_ASSERT(vkCreatePipelineLayout(device.device, &info, nullptr, &out->layout));
    
            VkShaderModule comp;
            VkShaderModuleCreateInfo compInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = compSpv.size(),
                .pCode = reinterpret_cast<const uint32_t*>(compSpv.data()),
            };
            VK_ASSERT(vkCreateShaderModule(device.device, &compInfo, nullptr, &comp));
    
            VkComputePipelineCreateInfo pipelineInfo{
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                    .module = comp,
                    .pName = "main",
                    .pSpecializationInfo = nullptr,
                },
                .layout = out->layout,
                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = -1,
            };
            VK_ASSERT(vkCreateComputePipelines(device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &out->pipeline));
            vkDestroyShaderModule(device.device, comp, nullptr);
        };
    
        createCompute(0, shaders::depth_tree_comp, &pass.depthTree.generate);
        createCompute(0, shaders::frust_cull_comp, &pass.earlyCull.frustumCull);
        createCompute(0, shaders::occl_cull_comp, &pass.earlyCull.occlusionCull);
        createCompute(0, shaders::gen_tiles_comp, &pass.lateCull.generateTiles);
        createCompute(0, shaders::gen_clusters_comp, &pass.lateCull.generateClusters);
        createCompute(0, shaders::tile_cull_comp, &pass.lateCull.lightCulling);
        createCompute(0, shaders::mesh_cull_comp, &pass.lateCull.meshCulling);
        
        auto createGraphics = [&](
            uint32_t pushData,
            std::span<const unsigned char> vertSpv,
            std::span<const unsigned char> fragSpv,
            VkFormat colorFormat,
            VkFormat depthFormat,
            VkCompareOp depthOp,
            bool depthWrite,
            auto* out
        ) {
            VkPushConstantRange range{ 
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = pushData
            };
            VkPipelineLayoutCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = static_cast<uint32_t>(sets.size()),
                .pSetLayouts = sets.data(),
                .pushConstantRangeCount = pushData ? 1u : 0u,
                .pPushConstantRanges = pushData ? &range : nullptr,
            };
            VK_ASSERT(vkCreatePipelineLayout(device.device, &info, nullptr, &out->layout));
            
            VkShaderModule vert, frag; 
            VkShaderModuleCreateInfo vertInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = vertSpv.size(),
                .pCode = reinterpret_cast<const uint32_t*>(vertSpv.data()),
            };
            VK_ASSERT(vkCreateShaderModule(device.device, &vertInfo, nullptr, &vert));
    
            VkShaderModuleCreateInfo fragInfo{
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = fragSpv.size(),
                .pCode = reinterpret_cast<const uint32_t*>(fragSpv.data()),
            };
            VK_ASSERT(vkCreateShaderModule(device.device, &fragInfo, nullptr, &frag));
            
            auto stages = std::to_array<VkPipelineShaderStageCreateInfo>({ 
                { 
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vert,
                    .pName = "main"
                },
                {   
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = frag,
                    .pName = "main"
                }
            });
    
            auto bindings = std::to_array<VkVertexInputBindingDescription>({
                { 
                    .binding = 0,
                    .stride = sizeof(primitives::Vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                }
            });
            // TODO: add normal / uv attributes with offsetof(primitives::Vertex, ...) once the layout is known
            auto attributes = std::to_array<VkVertexInputAttributeDescription>({
                {
                    .location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT, // position
                    .offset = 0
                }
            });
    
            VkPipelineVertexInputStateCreateInfo vertexInput{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size()),
                .pVertexBindingDescriptions = bindings.data(),
                .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size()),
                .pVertexAttributeDescriptions = attributes.data(),
            };
    
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE,
            };
    
            // viewport/scissor are dynamic, only the counts matter
            VkPipelineViewportStateCreateInfo viewportState{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .scissorCount = 1,
            };
    
            VkPipelineRasterizationStateCreateInfo rasterization{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_BACK_BIT,
                .frontFace = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = VK_FALSE,
                .lineWidth = 1.0f, // must be 1.0 unless wideLines is enabled
            };
    
            VkPipelineMultisampleStateCreateInfo multisample{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable = VK_FALSE,
            };
    
            VkPipelineDepthStencilStateCreateInfo depthStencil{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .depthTestEnable = VK_TRUE,
                .depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE,
                .depthCompareOp = depthOp,
                .depthBoundsTestEnable = VK_FALSE,
                .stencilTestEnable = VK_FALSE,
            };
    
            VkPipelineColorBlendAttachmentState blendAttachment{
                .blendEnable = VK_FALSE,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            };
            
            bool hasColor = colorFormat != VK_FORMAT_UNDEFINED;
    
            VkPipelineColorBlendStateCreateInfo blend{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .attachmentCount = hasColor ? 1u : 0u,
                .pAttachments = hasColor ? &blendAttachment : nullptr,
            };
    
            auto dynamics = std::to_array<VkDynamicState>({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR });
            VkPipelineDynamicStateCreateInfo dynamicState{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = static_cast<uint32_t>(dynamics.size()),
                .pDynamicStates = dynamics.data(),
            };
    
            // dynamic rendering (Vulkan 1.3): no VkRenderPass
            VkPipelineRenderingCreateInfo rendering{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .viewMask = 0,
                .colorAttachmentCount = hasColor ? 1u : 0u,
                .pColorAttachmentFormats = hasColor ? &colorFormat : nullptr,
                .depthAttachmentFormat = depthFormat,
                .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
            };
    
            VkGraphicsPipelineCreateInfo pipelineInfo{
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &rendering,
                .flags = 0,
                .stageCount = static_cast<uint32_t>(stages.size()),
                .pStages = stages.data(),
                .pVertexInputState = &vertexInput,
                .pInputAssemblyState = &inputAssembly,
                .pTessellationState = nullptr,
                .pViewportState = &viewportState,
                .pRasterizationState = &rasterization,
                .pMultisampleState = &multisample,
                .pDepthStencilState = &depthStencil,
                .pColorBlendState = &blend,
                .pDynamicState = &dynamicState,
                .layout = out->layout,
                .renderPass = VK_NULL_HANDLE,
                .subpass = 0,
                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = -1,
            };
            VK_ASSERT(vkCreateGraphicsPipelines(device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &out->pipeline));
    
            vkDestroyShaderModule(device.device, vert, nullptr);
            vkDestroyShaderModule(device.device, frag, nullptr);
        };
    
        createGraphics(0, shaders::depth_vert, shaders::depth_frag, VK_FORMAT_UNDEFINED, kDepthFormat, VK_COMPARE_OP_LESS, true, &pass.depthOnly.depthPass);
        createGraphics(0, shaders::forward_vert, shaders::forward_frag, kColorFormat, kDepthFormat, VK_COMPARE_OP_EQUAL, false, &pass.forward.forward);    
    }
    
    for (auto& frame : frames) {
        { // create fence
            VkFenceCreateInfo fenceInfo{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            };
            VK_ASSERT(vkCreateFence(device.device, &fenceInfo, nullptr, &frame.inFlight));
        }

        { // create command buffers
            auto createCmd = [&](VkCommandPool pool, VkCommandBuffer& out) {
                VkCommandBufferAllocateInfo cmdInfo{
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .commandPool = pool,
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount = 1,
                };
                VK_ASSERT(vkAllocateCommandBuffers(device.device, &cmdInfo, &out));
            };

            createCmd(device.queue.compute.pool,  frame.pass.depthTree.cmd);
            createCmd(device.queue.compute.pool,  frame.pass.earlyCull.cmd);
            createCmd(device.queue.graphics.pool, frame.pass.depthOnly.cmd);
            createCmd(device.queue.compute.pool,  frame.pass.lateCull.cmd);
            createCmd(device.queue.graphics.pool, frame.pass.forward.cmd);
        }

        { // create semaphores
            VkSemaphoreCreateInfo semaInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
            };

            VK_ASSERT(vkCreateSemaphore(device.device, &semaInfo, nullptr, &frame.pass.depthTree.finished));
            VK_ASSERT(vkCreateSemaphore(device.device, &semaInfo, nullptr, &frame.pass.earlyCull.finished));
            VK_ASSERT(vkCreateSemaphore(device.device, &semaInfo, nullptr, &frame.pass.depthOnly.finished));
            VK_ASSERT(vkCreateSemaphore(device.device, &semaInfo, nullptr, &frame.pass.lateCull.finished));
            VK_ASSERT(vkCreateSemaphore(device.device, &semaInfo, nullptr, &frame.pass.forward.finished));
        }

        { // create buffers
            auto createBuffer = [&](std::initializer_list<uint32_t> queues, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags flags, auto* out) {
                std::vector<uint32_t> families(queues);
                std::ranges::sort(families);
                families.erase(std::ranges::unique(families).begin(), families.end());

                VkBufferCreateInfo bufferInfo{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .size = size,
                    .usage = usage,
                    .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                    .pQueueFamilyIndices = families.data(),
                };
        
                VmaAllocationCreateInfo allocInfo{
                    .flags = flags,
                    .usage = VMA_MEMORY_USAGE_AUTO,
                };
        
                
                VmaAllocationInfo info{};
                VK_ASSERT(vmaCreateBuffer(device.allocator, &bufferInfo, &allocInfo, &out->buffer, &out->memory, &info));

                if constexpr (requires{ out->mapped; }) {
                    out->mapped = reinterpret_cast<decltype(out->mapped)>(info.pMappedData);
                }
            };

            createBuffer(
                { device.queue.graphics.family, device.queue.compute.family, device.queue.transfer.family }, 
                sizeof(primitives::Camera), 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, 
                &frame.camera
            );
            createBuffer(
                { device.queue.graphics.family, device.queue.compute.family, device.queue.transfer.family },
                sizeof(VkDrawIndexedIndirectCommand) * kMaxInstances,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                0, 
                &frame.instance.manifest
            );
            createBuffer(
                { device.queue.graphics.family, device.queue.compute.family },
                sizeof(uint32_t) * kMaxInstances,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                0, 
                &frame.instance.early
            );
            createBuffer(
                { device.queue.graphics.family, device.queue.compute.family },
                sizeof(uint32_t) * kMaxInstances,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                0, 
                &frame.instance.late
            );
            createBuffer(
                { device.queue.graphics.family, device.queue.compute.family },
                sizeof(uint32_t) * kMaxInstances,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                0, 
                &frame.instance.cluster
            );
            createBuffer(
                { device.queue.graphics.family, device.queue.compute.family },
                sizeof(primitives::Light) * kMaxLights,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                0, 
                &frame.light.manifest
            );
            createBuffer(
                { device.queue.graphics.family, device.queue.compute.family },
                sizeof(uint32_t) * kMaxLightsPerTile * kMaxTiles,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                0, 
                &frame.light.tile
            );
        }
        
        { // create attachments
            auto createImage = [&](
                VkImageAspectFlagBits aspect, // VK_IMAGE_ASPECT_COLOR_BIT
                VkFormat format,
                VkSampleCountFlagBits samples,
                VkImageUsageFlags usage,
                std::initializer_list<uint32_t> queues,
                auto* out
            ) {
                std::vector<uint32_t> families(queues);
                std::ranges::sort(families);
                families.erase(std::ranges::unique(families).begin(), families.end());

                uint32_t mipCount = 0;
                if constexpr (requires{ out->mips; }) {
                    mipCount = sizeof(out->mips) / sizeof(VkImageView);
                }

                VkImageLayout layout;
                switch (aspect) {
                    case VK_IMAGE_ASPECT_COLOR_BIT: layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; break;
                    case VK_IMAGE_ASPECT_DEPTH_BIT: layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL; break;
                    default: layout = VK_IMAGE_LAYOUT_GENERAL; break;
                }

                VkImageCreateInfo imageInfo{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format = format,
                    .extent = { settings.resolution.x, settings.resolution.y, 1 },
                    .mipLevels = mipCount,
                    .arrayLayers = 1,
                    .samples = samples,
                    .tiling = VK_IMAGE_TILING_OPTIMAL,
                    .usage = usage,
                    .sharingMode = families.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                    .queueFamilyIndexCount = static_cast<uint32_t>(families.size()),
                    .pQueueFamilyIndices = families.data(),
                    .initialLayout = layout,
                };
                
                VmaAllocationCreateInfo allocInfo {
                    .flags = 0,
                    .usage = VMA_MEMORY_USAGE_AUTO,
                };

                VK_ASSERT(vmaCreateImage(device.allocator, &imageInfo, &allocInfo, &out->image, &out->memory, nullptr));

                VkImageViewCreateInfo viewInfo{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .image = out->image,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = format,
                    .subresourceRange = { 
                        .aspectMask = aspect,
                        .baseMipLevel = 0,
                        .levelCount = mipCount,
                        .baseArrayLayer = 0,
                        .layerCount = 1
                    }  
                };

                VK_ASSERT(vkCreateImageView(device.device, &viewInfo, nullptr, &out->view));

                if constexpr (requires{ out->mips; }) {
                    for (uint32_t i = 0; i < mipCount; ++i) {
                        VkImageViewCreateInfo viewInfo{
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .pNext = nullptr,
                            .flags = 0,
                            .image = out->image,
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = format,
                            .subresourceRange = { 
                                .aspectMask = aspect,
                                .baseMipLevel = i,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1
                            }
                        };
                        
                        VK_ASSERT(vkCreateImageView(device.device, &viewInfo, nullptr, &out->mips[i]));
                    }
                }


            };
            
            createImage(VK_IMAGE_ASPECT_COLOR_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, { device.queue.graphics.family }, &frame.attachment.color);
            createImage(VK_IMAGE_ASPECT_DEPTH_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, { device.queue.graphics.family, device.queue.compute.family }, &frame.attachment.depth);
        }

        { // create descriptor set
            VkDescriptorSetAllocateInfo allocInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = descriptor.pool,
                .descriptorSetCount = 1,
                .pSetLayouts = &descriptor.layout,
            };
            VK_ASSERT(vkAllocateDescriptorSets(device.device, &allocInfo, &frame.descriptor.set));

            VkDescriptorBufferInfo cameraInfo{ 
                .buffer = frame.camera.buffer, 
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            VkDescriptorBufferInfo instanceManifestInfo{ 
                .buffer = frame.instance.manifest.buffer, 
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            VkDescriptorBufferInfo instanceEarlyInfo{ 
                .buffer = frame.instance.early.buffer, 
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            VkDescriptorBufferInfo instanceLateInfo{ 
                .buffer = frame.instance.late.buffer, 
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            VkDescriptorBufferInfo instanceClusterInfo{ 
                .buffer = frame.instance.cluster.buffer, 
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            VkDescriptorBufferInfo lightManifestInfo{ 
                .buffer = frame.light.manifest.buffer, 
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            VkDescriptorBufferInfo lightTileInfo{ 
                .buffer = frame.light.tile.buffer, 
                .offset = 0,
                .range = VK_WHOLE_SIZE
            };
            VkDescriptorImageInfo depthTreeSamplerInfo{ 
                .sampler = attachment.depth.sampler,
                .imageView = frame.attachment.depth.view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            VkDescriptorImageInfo depthMipsInfos[kDepthMipCount];
            for (uint32_t i = 0; i < kDepthMipCount; ++i) {
                depthMipsInfos[i] = { 
                    .sampler = VK_NULL_HANDLE,  
                    .imageView = frame.attachment.depth.mips[i],  // TODO: 
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL
                };
            }

            auto writes = std::to_array<VkWriteDescriptorSet>({
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frame.descriptor.set,
                    .dstBinding = 0, .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &cameraInfo
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frame.descriptor.set,
                    .dstBinding = 1, .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &instanceManifestInfo
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frame.descriptor.set,
                    .dstBinding = 2, .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &instanceEarlyInfo
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frame.descriptor.set,
                    .dstBinding = 3, .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &instanceLateInfo
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frame.descriptor.set,
                    .dstBinding = 4, .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &instanceClusterInfo
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frame.descriptor.set,
                    .dstBinding = 5, .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &lightManifestInfo
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frame.descriptor.set,
                    .dstBinding = 6, .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .pBufferInfo = &lightTileInfo
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frame.descriptor.set,
                    .dstBinding = 7, .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &depthTreeSamplerInfo
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = frame.descriptor.set,
                    .dstBinding = 8, .dstArrayElement = 0,
                    .descriptorCount = kDepthMipCount,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = depthMipsInfos
                },
            });
            vkUpdateDescriptorSets(device.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }
    }
}

void engine::Render::destroy(const Core &core, const Device &device) {
    vkDeviceWaitIdle(device.device);

    for (auto& frame : frames) {
        vkDestroyFence(device.device, frame.inFlight, nullptr);

        vkFreeDescriptorSets(device.device, device.descriptor.pool, 1, &frame.descriptor.set);

        vkFreeCommandBuffers(device.device, device.queue.compute.pool,  1, &frame.pass.depthTree.cmd);
        vkFreeCommandBuffers(device.device, device.queue.compute.pool,  1, &frame.pass.earlyCull.cmd);
        vkFreeCommandBuffers(device.device, device.queue.graphics.pool, 1, &frame.pass.depthOnly.cmd);
        vkFreeCommandBuffers(device.device, device.queue.compute.pool,  1, &frame.pass.lateCull.cmd);
        vkFreeCommandBuffers(device.device, device.queue.graphics.pool, 1, &frame.pass.forward.cmd);

        vkDestroySemaphore(device.device, frame.pass.depthTree.finished, nullptr);
        vkDestroySemaphore(device.device, frame.pass.earlyCull.finished, nullptr);
        vkDestroySemaphore(device.device, frame.pass.depthOnly.finished, nullptr);
        vkDestroySemaphore(device.device, frame.pass.lateCull.finished, nullptr);
        vkDestroySemaphore(device.device, frame.pass.forward.finished, nullptr);

        vmaDestroyBuffer(device.allocator, frame.camera.buffer, frame.camera.memory);
        vmaDestroyBuffer(device.allocator, frame.instance.manifest.buffer, frame.instance.manifest.memory);
        vmaDestroyBuffer(device.allocator, frame.instance.early.buffer, frame.instance.early.memory);
        vmaDestroyBuffer(device.allocator, frame.instance.late.buffer, frame.instance.late.memory);
        vmaDestroyBuffer(device.allocator, frame.instance.cluster.buffer, frame.instance.cluster.memory);
        vmaDestroyBuffer(device.allocator, frame.light.manifest.buffer, frame.light.manifest.memory);
        vmaDestroyBuffer(device.allocator, frame.light.tile.buffer, frame.light.tile.memory);

        for (uint32_t i = 0; i < kDepthMipCount; ++i) {
            vkDestroyImageView(device.device, frame.attachment.depth.mips[i], nullptr);
        }
        vkDestroyImageView(device.device, frame.attachment.depth.view, nullptr);
        vmaDestroyImage(device.allocator, frame.attachment.depth.image, frame.attachment.depth.memory);

        vkDestroyImageView(device.device, frame.attachment.color.view, nullptr);
        vmaDestroyImage(device.allocator, frame.attachment.color.image, frame.attachment.color.memory);
        
    }

    vkDestroyPipeline(device.device, pass.depthTree.generate.pipeline, nullptr);
    vkDestroyPipelineLayout(device.device, pass.depthTree.generate.layout, nullptr);
    
    vkDestroyPipeline(device.device, pass.earlyCull.frustumCull.pipeline, nullptr);
    vkDestroyPipelineLayout(device.device, pass.earlyCull.frustumCull.layout, nullptr);
    
    vkDestroyPipeline(device.device, pass.earlyCull.occlusionCull.pipeline, nullptr);
    vkDestroyPipelineLayout(device.device, pass.earlyCull.occlusionCull.layout, nullptr);
    
    vkDestroyPipeline(device.device, pass.depthOnly.depthPass.pipeline, nullptr);
    vkDestroyPipelineLayout(device.device, pass.depthOnly.depthPass.layout, nullptr);
    
    vkDestroyPipeline(device.device, pass.lateCull.generateTiles.pipeline, nullptr);
    vkDestroyPipelineLayout(device.device, pass.lateCull.generateTiles.layout, nullptr);
    
    vkDestroyPipeline(device.device, pass.lateCull.generateClusters.pipeline, nullptr);
    vkDestroyPipelineLayout(device.device,pass.lateCull.generateClusters.layout, nullptr);
    
    vkDestroyPipeline(device.device, pass.lateCull.lightCulling.pipeline, nullptr);
    vkDestroyPipelineLayout(device.device, pass.lateCull.lightCulling.layout, nullptr);
    
    vkDestroyPipeline(device.device, pass.lateCull.meshCulling.pipeline, nullptr);
    vkDestroyPipelineLayout(device.device, pass.lateCull.meshCulling.layout, nullptr);
    
    vkDestroyPipeline(device.device, pass.forward.forward.pipeline, nullptr);
    vkDestroyPipelineLayout(device.device, pass.forward.forward.layout, nullptr);

    vkDestroyDescriptorPool(device.device, descriptor.pool, nullptr);
    vkDestroyDescriptorSetLayout(device.device, descriptor.layout, nullptr);
}