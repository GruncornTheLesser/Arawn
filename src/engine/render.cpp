#define ARAWN_INCLUDE_VULKAN
#include <engine/render.h>
#include <engine/window.h>
#include <engine/device.h>
#include <engine/world.h>
#include <common/settings.h>
#include <nodes/camera.h>
#include <nodes/light.h>
#include <algorithm>
#include <array>
#include <vector>

using namespace arawn;
using namespace arawn::common;

void engine::Render::create(const Window& window, const Device& device, const World& world, const Swap& Swap, const Settings& settings) {
    frames.resize(settings.buffering == BufferingMode::DOUBLE ? 2 : 3);

    uint32_t attachmentMipCount = std::min<uint32_t>(maxMipLevels, std::bit_width(std::min(settings.resolution.x, settings.resolution.y)));
    struct { uint32_t x, y; } tileResolution {
        .x = (settings.resolution.x + settings.budget.tileSize - 1) / settings.budget.tileSize,
        .y = (settings.resolution.y + settings.budget.tileSize - 1) / settings.budget.tileSize,
    };
    uint32_t tileCount = tileResolution.x * tileResolution.y;
    uint32_t clusterCount = settings.budget.clusterResolution.x * settings.budget.clusterResolution.y;

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
                .descriptorCount = attachmentMipCount, 
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
                .descriptorCount = attachmentMipCount * n
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
        pass.depthTree.generate = resources::GenerateZTreePipeline(device, world, *this);
        pass.earlyCull.frustumCull = resources::FrustumCullPipeline(device, world, *this);
        pass.earlyCull.occlusionCull = resources::OcclusionCullPipeline(device, world, *this);
        pass.lateCull.generateTiles = resources::GenerateTilesPipeline(device, world, *this);
        pass.lateCull.generateClusters = resources::GenerateClustersPipeline(device, world, *this);
        pass.lateCull.lightCulling = resources::LightCullingPipeline(device, world, *this);
        pass.lateCull.meshCulling = resources::MeshCullingPipeline(device, world, *this);
        pass.depthOnly.depthPass = resources::DepthPipeline(device, world, *this);
        pass.forward.forward = resources::ForwardPipeline(device, world, *this);
    }
    
    { // create depth sampler
        VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipLodBias = 0.0,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1.0f,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS ,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_TRUE,
        };

        VK_ASSERT(vkCreateSampler(device.device, &samplerInfo, nullptr, &attachment.depth.sampler));
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
            frame.camera.create(
                device, sizeof(primitives::Camera), 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, 
                { device.queue.graphics.family, device.queue.compute.family, device.queue.transfer.family },
                reinterpret_cast<void**>(&frame.camera.mapped)
            );
             
            frame.instance.manifest.create(
                device, sizeof(VkDrawIndexedIndirectCommand) * settings.budget.maxInstances, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                0, 
                { device.queue.graphics.family, device.queue.compute.family, device.queue.transfer.family }
            );
            
            frame.instance.early.create(
                device, sizeof(uint32_t) * settings.budget.maxInstances, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                0, 
                { device.queue.graphics.family, device.queue.compute.family }
            );
            
            frame.instance.late.create(
                device, sizeof(uint32_t) * clusterCount * settings.budget.maxMeshesPerCluster, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                0, 
                { device.queue.graphics.family, device.queue.compute.family }
            );
            
            frame.instance.cluster.create(
                device, sizeof(uint32_t) * clusterCount, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                0, 
                { device.queue.graphics.family, device.queue.compute.family }
            );
            
            frame.light.manifest.create(
                device, sizeof(primitives::Light) * settings.budget.maxLights, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                0, 
                { device.queue.graphics.family, device.queue.compute.family }
            );
            
            frame.light.tile.create(
                device, sizeof(uint32_t) * settings.budget.maxLightPerTile * tileCount, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                0, 
                { device.queue.graphics.family, device.queue.compute.family }
            );
            
        }
        
        { // create attachments
            frame.attachment.color.create(device,
                settings.resolution.x, settings.resolution.y, 1, 1, 
                device.format.attachment.color, VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                0, 
                { device.queue.graphics.family, device.queue.present.family }
            );
            frame.attachment.color.view = frame.attachment.color.createView(device,
                device.format.attachment.color, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1
            );

            frame.attachment.depth.create(device,
                settings.resolution.x, settings.resolution.y, 1, attachmentMipCount, 
                device.format.attachment.depth, VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                0, 
                { device.queue.graphics.family, device.queue.present.family }
            );
            frame.attachment.depth.view = frame.attachment.depth.createView(device,  
                device.format.attachment.depth, VK_IMAGE_ASPECT_DEPTH_BIT, 0, attachmentMipCount
            );
            for (uint32_t i = 0; i < attachmentMipCount; ++i) {
                frame.attachment.depth.mips[i] = frame.attachment.depth.createView(device, 
                    device.format.attachment.depth, VK_IMAGE_ASPECT_DEPTH_BIT, i, 1
                );
            }
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
            VkDescriptorImageInfo depthMipsInfos[maxMipLevels];
            
            for (uint32_t i = 0; i < attachmentMipCount; ++i) {
                depthMipsInfos[i] = { 
                    .sampler = VK_NULL_HANDLE,  
                    .imageView = frame.attachment.depth.mips[i], 
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
                    .descriptorCount = attachmentMipCount,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = depthMipsInfos
                },
            });
            vkUpdateDescriptorSets(device.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }
    }
}

void engine::Render::destroy(const Device &device) {
    vkDeviceWaitIdle(device.device);

    for (auto& frame : frames) {
        vkDestroyFence(device.device, frame.inFlight, nullptr);

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

        frame.camera.destroy(device);
        frame.instance.manifest.destroy(device);
        frame.instance.early.destroy(device);
        frame.instance.late.destroy(device);
        frame.instance.cluster.destroy(device);
        frame.light.manifest.destroy(device);
        frame.light.tile.destroy(device);

        for (uint32_t i = 0; i < maxMipLevels; ++i) {
            vkDestroyImageView(device.device, frame.attachment.depth.mips[i], nullptr);
        }
        vkDestroyImageView(device.device, frame.attachment.depth.view, nullptr);
        frame.attachment.depth.destroy(device);

        vkDestroyImageView(device.device, frame.attachment.color.view, nullptr);
        frame.attachment.color.destroy(device);
        
    }

    vkDestroySampler(device.device, attachment.depth.sampler, nullptr);
    
    pass.depthTree.generate.destroy(device);
    pass.earlyCull.frustumCull.destroy(device);
    pass.earlyCull.occlusionCull.destroy(device);
    pass.depthOnly.depthPass.destroy(device);
    pass.lateCull.generateTiles.destroy(device);
    pass.lateCull.generateClusters.destroy(device);
    pass.lateCull.lightCulling.destroy(device);
    pass.lateCull.meshCulling.destroy(device);
    pass.forward.forward.destroy(device);

    vkDestroyDescriptorPool(device.device, descriptor.pool, nullptr);
    vkDestroyDescriptorSetLayout(device.device, descriptor.layout, nullptr);
}