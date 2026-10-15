#pragma once
#include <common/fwd.h>
#include <common/handle.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace arawn::nodes {
    struct Camera {
        struct CreateInfo {
            Handle<nodes::Node> node;
            float fov;
        };

        glm::mat4 projection;
    };
}