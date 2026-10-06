#pragma once
#include "handle.h"
#include "assert.h"
#include <queue>
#include <vector>
#include <algorithm>
#include <ranges>


namespace arawn::common {
	template<class T>
	class Pool {
        using handle_type = Handle<T>;
        using lookup_type = Handle<T>;

		template<typename ... Arg_Ts>
		Handle<T> create(Arg_Ts&& ... args) {
            handle_type hnd;

            if (!free_list.empty()) {
                uint32_t idx = static_cast<uint32_t>(free_list.back());
                free_list.pop_back();

                hnd = { idx, sparse[idx].version };
                sparse[idx].index = static_cast<uint32_t>(storage.size());
            } else {
                hnd = { static_cast<uint32_t>(sparse.size()), 0 };
                sparse.push_back({ static_cast<uint32_t>(storage.size()), 0 });
            }

            storage.emplace_back(std::forward<Arg_Ts>(args)...);
			packed.push_back(hnd);
			return hnd;
		}

		void destroy(Handle<T> hnd) {
			uint32_t pos = sparse[hnd.index].index;
            uint32_t end = static_cast<uint32_t>(storage.size() - 1);

            if (pos != end) {
                std::swap(storage[pos], storage[end]);
                std::swap(packed[pos], packed[end]);

                uint32_t idx = packed[pos].index;
                sparse[idx].index = pos;
            }

            storage.pop_back();
            packed.pop_back();

            sparse[hnd.index].index = 0xfffff;
            free_list.push_back(hnd.index);
		}

		void destroy(Handle<T> hnd, uint32_t frame_index) {
			uint32_t pos = sparse[hnd.index].index;
            uint32_t end = static_cast<uint32_t>(storage.size() - 1);

            if (pos != end) {
                std::swap(storage[pos], storage[end]);
                std::swap(packed[pos], packed[end]);

                uint32_t idx = packed[pos].index;
                sparse[idx].index = pos;
            }
            destroy_queue.push_back({ { hnd.index, frame_index }, std::move(storage.back()) });

            storage.pop_back();
            packed.pop_back();

            sparse[hnd.index].index = 0xfffff;
		}

        void advance(uint32_t current_frame) {
            while (!destroy_queue.empty()) {
                auto& curr = destroy_queue.front();

                if (curr.first.index != current_frame) {
                    return;
                }

                free_list.push_back(curr.first.index);
                destroy_queue.pop_front();
            }
        }

        void flush() {
            free_list.append_range(std::ranges::transform(destroy_queue, {}, [](auto& curr) { return curr.first.index; }));
            destroy_queue.clear();
        }

        void clear() {
            storage.clear();
            packed.clear();
            sparse.clear();
            free_list.clear();
            destroy_queue.clear();
        }

		bool alive(Handle<T> hnd) const {
			if (hnd.index >= sparse.size()) return false;
			Handle<T> ind = sparse[hnd.index];
			if (ind.version != hnd.index) return false;
			Handle<T> pck = packed[ind.index];
			return hnd == pck;
		}

		T& get(Handle<T> hnd) { return storage[sparse[hnd.index].index]; }
		const T& get(Handle<T> hnd) const { return storage[sparse[hnd.index].index]; }
		
		T& at(uint32_t idx) { return storage[idx]; }
		const T& at(uint32_t idx) const { return storage[idx]; }
		
	private:
        std::vector<lookup_type> sparse; // alive + dead handle mappings
        std::vector<handle_type> packed; // alive handle
        std::vector<T>           storage; // actual data
        std::vector<uint32_t>    free_list; // free handles
        std::queue<std::pair<handle_type, T>> destroy_queue;
	};
}