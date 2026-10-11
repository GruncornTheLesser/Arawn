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

namespace arawn::primitives {
    struct Camera {
        glm::mat4 view;
        glm::vec3 eye;
        float fov;
        // planes[6];
    };
}