#include "engine.h"
#include "window.h"
#include "scene/scene.h"

int main() {
    Window window(800u, 600u);
    
    //vk_renderer dynamic_renderer { engine, window };
    //vk_forward_render forward_renderer { engine, window };

    scene scene;
    
    while (!window.closed()) { }
}

/*
????
    VK_TYPE(VkSemaphore) imageAvailableSemaphore;
    VK_TYPE(VkSemaphore) renderFinishedSemaphore;
    VK_TYPE(VkFence) inFlightFence;

    struct renderer {
        struct program {
            VK_TYPE(VkRenderPass) renderpass;
            VK_TYPE(VkPipelineLayout) layout;
            VK_TYPE(VkPipeline) pipeline;
        };
        VK_TYPE(VkFramebuffer) Framebuffers[max_image_count];
        VK_TYPE(VkCommandPool) cmd_pool;
        VK_TYPE(VkCommandBuffer) cmd_buffer;
    
        void draw(scene& scene);
    };
*/

/* dynamic renderer
    presentation -> image_count, format, 
    inputs -> lights, meshes, camera,
    programs -> depth_prepass, geometry_pass, forward_pass, deferred_pass, generate_cluster_bb, generate_tile_bb, forward_plus_pass 
    vk_objs -> fences -> framebuffers
*/

/*
    Hierarchal 2D box colliders to check for inputs
*/