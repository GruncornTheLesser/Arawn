#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>


namespace arawn::pipeline {
    struct Forward {
        struct PushData { };
        
        void create(const core::Device& device, const attachment::Color& color, const attachment::Depth& depth);
        void destroy(const core::Device& device);

        
        VK_TYPE(VkPipeline) pipeline;
        VK_TYPE(VkPipelineLayout) layout;
    };
}