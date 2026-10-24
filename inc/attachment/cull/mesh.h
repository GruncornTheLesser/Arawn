#pragma once 
#include <common/fwd.h>
#include <common/vulkan.h>

namespace arawn::attachment::cull {
    struct Mesh {
        void create(const core::Device& device, const core::Frame& frame, const core::Settings& settings);
        void destroy(const core::Device& device);

        struct {
            VK_TYPE(VkBuffer) buffer[ARAWN_MAX_FRAMES];
            VK_TYPE(VmaAllocation) memory[ARAWN_MAX_FRAMES];
        } manifest; // count + mesh drawn per frame

        struct {
            VK_TYPE(VkBuffer) buffer[ARAWN_MAX_FRAMES];
            VK_TYPE(VmaAllocation) memory[ARAWN_MAX_FRAMES];
        } early; // early culled meshes drawn per frame

        struct {
            VK_TYPE(VkBuffer) buffer[ARAWN_MAX_FRAMES];
            VK_TYPE(VmaAllocation) memory[ARAWN_MAX_FRAMES];
        } late; // clusters + culled meshes
    };
}