#pragma once
#include <common/fwd.h>
#include <common/handle.h>
#include <vector>
#include <deque>


namespace arawn::common {
    struct Pool {
        struct slot {
            uint16_t version : 12;
            uint16_t active : ARAWN_MAX_FRAMES;
        };

        struct flush_range {
            struct sentinel { };

            struct iterator {
                using iterator_category = std::input_iterator_tag;
                using value_type = uint32_t;
                using difference_type = std::ptrdiff_t;
                
                iterator() = default;
                iterator(Pool* pool);
                value_type operator*() const;

                iterator& operator++();
                iterator operator++(int);

                friend bool operator==(const iterator& lhs, sentinel rhs);

            private:
                Pool* pool;
                uint32_t cached;
            };
            
            flush_range(Pool* pl);

            iterator begin() const;
            sentinel end() const;

        private:
            Pool* pool;
        };

        Handle<void> create();
        void destroy(Handle<void> hnd);

        flush_range flush(uint32_t idx);
        flush_range clear();

        bool alive(Handle<void> hnd) const;

    private:
        std::vector<slot> slots;
        std::deque<uint32_t> free;
    };

    bool operator==(const Pool::flush_range::iterator& lhs, Pool::flush_range::sentinel rhs);
}

/*
    template<typename T>
	struct Pool {
        struct slot_type {
            T data;
            uint16_t version : 12 = 0;
            uint16_t active : 4 = 0;
        };

        using handle_type = Handle<T>;
        struct framebuffer_type {
            uint32_t size() const { return 2; }
            uint32_t index() const { return 1; }
        };

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

*/