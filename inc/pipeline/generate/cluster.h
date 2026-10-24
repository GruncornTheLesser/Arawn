#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>


namespace arawn::pipeline::gen {
    struct Cluster {
        struct PushData { };
        
        void create(const core::Device& device);
        void destroy(const core::Device& device);

        
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkPipelineLayout) layout;
    };
}