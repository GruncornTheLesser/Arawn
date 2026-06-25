#pragma once
#include <render/core/vulkan.h>
#include <display/window.h>
#include <memory_resource>

namespace Arawn {
    struct Swapchain {
        struct Info {
            Window& window;
            bool triple_buffered : 1 = false;
            bool vsync : 1 = true;
            bool low_latency : 1 = true;
        };

        struct Registry {
            struct Builder;

            Registry(uint32_t frame_count, const Registry::Builder& builder, std::pmr::monotonic_buffer_resource allocator);
            ~Registry() noexcept;
        };

        Swapchain(Info&& info);
        Swapchain& operator=(Info&& info);

        ~Swapchain();
        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
        Swapchain(Swapchain&&);
        Swapchain& operator=(Swapchain&&);


        VK_ENUM(VkFormat) format;
        VK_ENUM(VkColorSpaceKHR) colour;
        VK_ENUM(VkPresentModeKHR) present;

        VK_TYPE(VkSurfaceKHR)   surface;
        VK_TYPE(VkSwapchainKHR) swapchain;
    };

    struct Swapchain::Registry::Builder {
        
    };
}