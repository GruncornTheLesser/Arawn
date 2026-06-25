#pragma once
#include <render/core/vulkan.h>
#include <render/core/glm.h>

namespace Arawn {
    enum Queue { GRAPHICS, COMPUTE, TRANSFER, PRESENT };

    class Engine {
    public:
        struct Info { 
            const char* app_name = nullptr;
            const char* device_name = nullptr;
        };

        Engine(const Info& info);
        ~Engine();

        Engine(Engine&&) = delete;
        Engine(const Engine&) = delete;
        Engine& operator=(Engine&&) = delete;
        Engine& operator=(const Engine&) = delete;

        VK_TYPE(VkInstance) instance;  // vulkan instance

        // TODO: device selection after initialization
        VK_TYPE(VkPhysicalDevice) gpu; // selected gpu
        VK_TYPE(VkDevice) device;      // logical device
        
        // TODO: only initialize on unique queues when multithreaded rendering
        struct Queue {
            uint32_t               family; // queue family index
            uint32_t               index;  // device queue index
            VK_TYPE(VkQueue)       queue;  // device queue
            VK_TYPE(VkCommandPool) pool;   // command pool
        } queue[4];

        VK_TYPE(VmaAllocator) allocator;
    };

    extern Engine engine;
}

/*

    class Resource {
        struct Info {
            
        };
    };

    class Renderer {
        struct Domain {
            uint32_t count;
            uint32_t index;
            uint32_t phase;
            uint32_t period;
        } frame, swapchain;
        
        struct Frame {
            VkImage depth_attachment;
            VkImageView depth_attachment_view;
            VmaAllocation depth_attachment_memory;

            VkImage color_attachment;
            VkImageView color_attachment_view;
            VmaAllocation color_attachment_memory;


        } *frames;
        void render() {
            VkFormat* color_attachment_formats;
            VkFormat depth_attachment_format;
            VkPipelineRenderingCreateInfo dynamic_pipeline_info {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .pNext = nullptr,
                .viewMask = 0,
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = color_attachment_formats,
                .depthAttachmentFormat = depth_attachment_format,
            };

            VkGraphicsPipelineCreateInfo graphics_create{
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &dynamic_pipeline_info,
                .flags = ,
                .stageCount = ,
                .pStages = ,
                .pVertexInputState = ,
                .pInputAssemblyState = ,
                .pTessellationState = ,
                .pViewportState = ,
                .pRasterizationState = ,
                .pMultisampleState = ,
                .pDepthStencilState = ,
                .pColorBlendState = ,
                .pDynamicState = ,
                .layout = ,
                .renderPass = VK_NULL_HANDLE,
                .subpass = ,
                .basePipelineHandle = ,
                .basePipelineIndex = ,
            };
            
            
            VkCommandBuffer cmd;
            VkPipelineLayout layout;
            VkDescriptorSet* sets;

            VkRenderingAttachmentInfo color_attachment_info {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = nullptr,
                .imageLayout = ,
                .resolveMode = ,
                .resolveImageView = ,
                .resolveImageLayout = ,
                .loadOp = ,
                .storeOp = ,
                .clearValue = ,
            };

            VkRenderingAttachmentInfo depth_attachment_info {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = nullptr,
                .imageLayout = ,
                .resolveMode = ,
                .resolveImageView = ,
                .resolveImageLayout = ,
                .loadOp = ,
                .storeOp = ,
                .clearValue = ,
            };
        
            VkRenderingInfo render_info{
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .pNext = nullptr,
                .flags = 0,
                .renderArea = { {}, { 800, 600 } },
                .layerCount = 1,
                .viewMask = 0,
                .colorAttachmentCount = 1,
                .pColorAttachments = &color_attachment_info,
                .pDepthAttachment = &depth_attachment_info,
                .pStencilAttachment = nullptr,
            };

            vkCmdBeginRendering(cmd, &render_info);

            // draw scene

            vkCmdEndRendering(cmd);
            
    
        }
    };
*/