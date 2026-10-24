#pragma once
#include <common/fwd.h>
#include <common/vulkan.h>
#include <common/handle.h>
#include <vector>
#include <deque>


namespace arawn::common {
    struct Pool {
        using index_type = ARAWN_INDEX_TYPE;
        using version_type = ARAWN_VERSION_TYPE;
        struct slot_type { version_type version; };
        using iter_type = std::deque<index_type>::const_iterator;
        using flush_range_type = std::ranges::subrange<iter_type, iter_type>;

        [[nodiscard]] Handle<void> create_resource() {
            if (inactive == 0) {
                uint32_t index = static_cast<uint32_t>(slots.size());
                slots.push_back({ .version = 0 });
                return { index, 0 };
            } else {
                uint32_t idx = queue.front();
                queue.pop_front();
                --inactive;
                
                slot_type& slot = slots[idx];

                return { idx, slot.version };
            }
        }

        void destroy_resource(Handle<void> hnd) {
            if (!alive(hnd)) return;
            slot_type& slot = slots[hnd.index];
            ++slot.version;
            ++pending[ARAWN_MAX_FRAMES - 1];
            queue.push_back(hnd.index);
        }

        [[nodiscard]] bool alive(Handle<void> hnd) const {
            return hnd.index < slots.size() && slots[hnd.index].version == hnd.version;
        }
        
        flush_range_type flush() {
            uint32_t begin = inactive;
            uint32_t end = begin + pending[0];

            inactive += pending[0];
            for (uint32_t i = 0; i < ARAWN_MAX_FRAMES - 1; ++i) {
                pending[i] = pending[i + 1];
            }
            pending[ARAWN_MAX_FRAMES - 1] = 0;

            return { queue.begin() + begin, queue.begin() + end };
        }

        flush_range_type flush_all() {
            uint32_t begin = inactive;

            for (uint32_t i = 0; i < ARAWN_MAX_FRAMES; ++i) {
                inactive += pending[i];
                pending[i] = 0;
            }

            return { queue.begin() + begin, queue.begin() + inactive };
        }

        uint32_t inactive{};
        uint32_t pending[ARAWN_MAX_FRAMES]{};
        std::deque<index_type> queue{};
        std::vector<slot_type> slots{};
    };
}