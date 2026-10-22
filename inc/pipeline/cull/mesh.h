#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>


namespace arawn::pipeline::cull {
    struct Mesh {
        struct PushData { };

        void create(const core::Device& device);
        void recreate(const core::Device& device);
        void destroy(const core::Device& device);

        
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkPipelineLayout) layout;
    };
}