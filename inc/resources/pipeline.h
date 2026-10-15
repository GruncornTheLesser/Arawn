#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <span>

namespace arawn::resources {
    struct Pipeline {
        void createCompute(
            const engine::Device& device,
            std::span<const unsigned char> compSpv,
            std::span<const VK_TYPE(VkDescriptorSetLayout)> sets,
            uint32_t pushDataSize
        );

        void destroy(const engine::Device& device);

        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkPipelineLayout) layout;
    };

    struct GenerateZTreePipeline : Pipeline { 
        struct PushData { uint32_t data; };
        GenerateZTreePipeline(const engine::Device& device, const engine::World& world, const engine::Render& render);
    };
    struct FrustumCullPipeline : Pipeline { 
        struct PushData { uint32_t data; };
        FrustumCullPipeline(const engine::Device& device, const engine::World& world, const engine::Render& render);
    };
    struct OcclusionCullPipeline : Pipeline { 
        struct PushData { uint32_t data; };
        OcclusionCullPipeline(const engine::Device& device, const engine::World& world, const engine::Render& render);
    }; // reprojection cull
    struct GenerateTilesPipeline : Pipeline { 
        struct PushData { uint32_t data; };
        GenerateTilesPipeline(const engine::Device& device, const engine::World& world, const engine::Render& render);
    };
    struct GenerateClustersPipeline : Pipeline { 
        struct PushData { uint32_t data; };
        GenerateClustersPipeline(const engine::Device& device, const engine::World& world, const engine::Render& render);
    };
    struct LightCullingPipeline : Pipeline { 
        struct PushData { uint32_t data; };
        LightCullingPipeline(const engine::Device& device, const engine::World& world, const engine::Render& render);
    };
    struct MeshCullingPipeline : Pipeline { 
        struct PushData { uint32_t data; };
        MeshCullingPipeline(const engine::Device& device, const engine::World& world, const engine::Render& render);
    };
    struct DepthPipeline : Pipeline {
        struct PushData { uint32_t data; };
        DepthPipeline(const engine::Device& device, const engine::World& world, const engine::Render& render);
    };
    struct ForwardPipeline : Pipeline {
        struct PushData { uint32_t data; };
        ForwardPipeline(const engine::Device& device, const engine::World& world, const engine::Render& render);
    };
}