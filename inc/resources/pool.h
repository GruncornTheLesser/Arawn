#pragma once
#include <common/handle.h>
#include <vector>
#include <deque>


namespace arawn::common {
    template<typename T>
	class Pool {
        struct slot_type {
            T data;
            uint16_t version : 12 = 0;
            uint16_t active : 4 = 0;
        };

        using handle_type = Handle<T>;
        struct framebuffer_type { uint32_t size() const { return 2; } uint32_t index() const { return 1; } }; // placeholder

        template<typename ... arg_Ts>
        handle_type create(const framebuffer_type& framebuffer, arg_Ts&& ... args) {
            uint32_t index;
            if (!free.empty() && !slots[free.back()].active) {
                index = free.back(); 
                free.pop_back();
            } else {
                index = static_cast<uint32_t>(slots.size());
                slots.emplace_back();
            }
            slot_type& slot = slots[index];
            slot.data.create(std::forward<arg_Ts>(args)...);
            
            slot.active = (1u << framebuffer.size()) - 1u; // mark active for all frames
            return handle_type{ index, slot.version };
        }

        void destroy(handle_type hnd) {
            if (!alive(hnd)) return;
            slot_type& slot = slots[hnd.index];
            slot.version += 1;
            free.push_front(hnd.index);
        }
        template<typename ... arg_Ts>
        void flush(const framebuffer_type& framebuffer, arg_Ts&& ... args) {
            uint32_t frame_mask = (1 << framebuffer.index());
            for (uint32_t index : free) {
                slot_type& slot = slots[index];
                
                if ((slot.active & frame_mask) == 0)
                    break;

                slot.active &= ~frame_mask;
                
                if (slot.active) {
                    continue;
                }
                slot.data.destroy(args...);
            }
        }

        template<typename ... arg_Ts>
        void flush_all(arg_Ts&& ... args) {
            for (auto& slot : slots) {
                if (slot.active) {
                    slot.data.destroy(args...);
                }
            }
            slots.clear();
            free.clear();
        }


        bool alive(handle_type hnd) const {
            if (hnd.index >= slots.size()) return false;
            const slot_type& slot = slots[hnd.index];
            return slot.active != 0 && slot.version == hnd.version;
        }

        T& get(handle_type hnd) { return slots[hnd.index].data; }
        const T& get(handle_type hnd) const { return slots[hnd.index].data; }
	private:
        std::vector<slot_type> slots;
        std::deque<uint32_t> free;
	};
}