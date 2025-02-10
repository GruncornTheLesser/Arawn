#include "scene/scene.h"
#include "glm/gtc/quaternion.hpp"

void transform_update(entt::registry& reg, entt::entity e) {
    reg.get_or_emplace<transform::update_tag>(e);
}

scene::scene() {
    on_update<transform>().connect<transform_update>();
}

void scene::update_hierarchy() {
    for (auto [e, curr] : view<hierarchy>().each()) {
        auto* next = try_get<hierarchy>(curr.parent);
        if (next != nullptr && &curr > next) {
            storage<hierarchy>().swap_elements(e, curr.parent);
        }
    }
}

void scene::update_transform() {
    if (view<transform::update_tag>().empty()) return; 
    
    // update local transforms 
    for (auto [e, trn] : view<transform, transform::update_tag>().each()) {
        trn.local = glm::mat4_cast(trn.rotation);
        trn.local = glm::scale(trn.local, trn.scale);
        trn.local = glm::translate(trn.local, trn.position);
    }

    clear<transform::update_tag>();

    // update world transforms
    // root nodes
    for (auto [e, trn] : view<transform>(entt::exclude<hierarchy>).each()) {
        trn.world = trn.local;
    }

    // branch nodes
    for (auto [e, hierarchy, trn] : view<hierarchy, transform>().each()) {
        auto* parent_trn = try_get<transform>(hierarchy.parent);
        trn.world = parent_trn != nullptr ? parent_trn->world * trn.local : trn.local;
    }
}