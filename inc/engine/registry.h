#pragma once
#include <cstdint>
#include <array>
#include <functional>

namespace arawn::engine {
    namespace meta {
		template<typename ... Ts> struct type_list {};

		template<typename F> struct func_args;
		template<typename ret_T, typename T, typename ... arg_Ts> struct func_args<ret_T(T::*)(arg_Ts...)>                { using type = type_list<std::remove_cvref_t<arg_Ts>...>; };
		template<typename ret_T, typename T, typename ... arg_Ts> struct func_args<ret_T(T::*)(arg_Ts...) noexcept>       { using type = type_list<std::remove_cvref_t<arg_Ts>...>; };
		template<typename ret_T, typename T, typename ... arg_Ts> struct func_args<ret_T(T::*)(arg_Ts...) const>          { using type = type_list<std::remove_cvref_t<arg_Ts>...>; };
		template<typename ret_T, typename T, typename ... arg_Ts> struct func_args<ret_T(T::*)(arg_Ts...) const noexcept> { using type = type_list<std::remove_cvref_t<arg_Ts>...>; };
		template<typename ret_T, typename ... arg_Ts>             struct func_args<ret_T(*)(arg_Ts...)>                   { using type = type_list<std::remove_cvref_t<arg_Ts>...>; };
		template<typename ret_T, typename ... arg_Ts>             struct func_args<ret_T(*)(arg_Ts...) noexcept>          { using type = type_list<std::remove_cvref_t<arg_Ts>...>; };
		template<typename F> using func_args_t = typename func_args<F>::type;

		template<typename F> struct func_class;
		template<typename ret_T, typename T, typename ... arg_Ts> struct func_class<ret_T(T::*)(arg_Ts...)>                { using type = T; };
		template<typename ret_T, typename T, typename ... arg_Ts> struct func_class<ret_T(T::*)(arg_Ts...) noexcept>       { using type = T; };
		template<typename ret_T, typename T, typename ... arg_Ts> struct func_class<ret_T(T::*)(arg_Ts...) const>          { using type = T; };
		template<typename ret_T, typename T, typename ... arg_Ts> struct func_class<ret_T(T::*)(arg_Ts...) const noexcept> { using type = T; };
		template<typename F> using func_class_t = typename func_class<F>::type;

        template<typename ... Ts> struct concat;
        template<> struct concat<> { using type = meta::type_list<>; };
        template<typename T> struct concat<T> { using type = T; };
        template<typename ... Ts, typename ... Us, typename ... Vs> struct concat<type_list<Ts...>, type_list<Us...>, Vs...> : concat<type_list<Ts..., Us...>, Vs...> { };
        template<typename ... Ts> using concat_t = typename concat<Ts...>::type;

        template<typename T, typename ... Ts> static constexpr std::size_t index_of() {
            if (std::size_t i = -1; ((++i, std::is_same_v<T, Ts>) || ...)) return i;
            return -1;
        }
        
	}

	template<std::size_t N>
	struct Bitmask {
		static_assert(N > 0);
		static constexpr std::size_t W = 64, M = (N + W - 1) / W;
		std::array<uint64_t, M> bits{};

		constexpr void set(std::size_t n, bool v = true) { const uint64_t m = uint64_t{1} << (n % W); if (v) bits[n / W] |= m; else bits[n / W] &= ~m; }
		constexpr bool at(std::size_t n) const { return (bits[n / W] >> (n % W)) & 1; }
		constexpr bool any() const { for (auto b : bits) if (b) return true; return false; }
		constexpr Bitmask& operator|=(const Bitmask& o) { for (std::size_t i = 0; i < M; ++i) bits[i] |= o.bits[i]; return *this; }
		constexpr Bitmask& operator&=(const Bitmask& o) { for (std::size_t i = 0; i < M; ++i) bits[i] &= o.bits[i]; return *this; }
		constexpr bool subset_of(const Bitmask& o) const { for (std::size_t i = 0; i < M; ++i) if (bits[i] & ~o.bits[i]) return false; return true; }
	};

	template<typename ... Ts>
	struct Registry {
		static constexpr std::size_t N = sizeof...(Ts);
		
		using mask = Bitmask<N>;
		using order = std::array<std::size_t, N>;
		
		static_assert(N > 0);

		struct graph {
			template<typename ... Ds>
			static constexpr mask mask_of(meta::type_list<Ds...>) { mask m; (m.set(meta::index_of<Ds, Ts...>()), ...); return m; }

			template<typename T> static constexpr mask create_deps() { return mask_of(meta::func_args_t<decltype(&T::create)>{}); }
			template<typename T> static constexpr mask recreate_deps() { return mask_of(meta::func_args_t<decltype(&T::recreate)>{}); }
			template<typename T> static constexpr mask destroy_deps() { return mask_of(meta::func_args_t<decltype(&T::destroy)>{}); }

			static_assert((recreate_deps<Ts>().subset_of(create_deps<Ts>()) && ...), "recreate takes a resource that create does not"); 
            static_assert((destroy_deps<Ts>().subset_of(create_deps<Ts>()) && ...), "destroy takes a resource that create does not");

			static constexpr std::array<mask, N> dependencies = { create_deps<Ts>()... };

			static constexpr std::array<mask, N> dependents = [] {
				std::array<mask, N> out{};
				for (std::size_t i = 0; i < N; ++i) for (std::size_t j = 0; j < N; ++j) {
                    if (dependencies[i].at(j)) {
                        out[j].set(i);
                    }
				}
				return out;
			}();

			static constexpr order topology = [] {
				order out{};
				mask done;
				for (std::size_t k = 0; k < N; ++k) {
					
                    bool found = false;
					for (std::size_t i = 0; i < N && !found; ++i) {
						if (done.at(i)) continue;
						
                        if (dependencies[i].at(i)) throw "resource depends on itself";
						
                        bool ready = true;
						for (std::size_t j = 0; j < N; ++j)
							if (dependencies[i].at(j) && !done.at(j)) {
                                ready = false;
                            }
						
                            if (ready) {
                                done.set(i);
                                out[k] = i;
                                found = true;
                            }
					}
					if (!found) throw "resource dependency cycle";
				}
				return out;
			}();
		};

		Registry() = default;
		explicit Registry(Ts&& ... rs) : resources(std::move(rs)...) {}

		Registry(const Registry&) = delete;
		Registry& operator=(const Registry&) = delete;

		~Registry() { destroy(); }

		void create() {
			for (std::size_t j = 0; j < graph::topology.size(); ++j) {
                if (std::size_t i = graph::topology[i]; !live.at(i)) {
                    create_table[i](*this); 
                    live.set(i, true);
                }
            }
		}

		void destroy() {
            for (std::size_t j = 0; j-- > graph::topology.size();) {
				if (std::size_t i = graph::topology[i]; !live.at(i)) {
					live.set(i, false);
					destroy_table[i](*this);
				}
			}
		}

		void recreate(mask pending) {
			for (std::size_t i : graph::topology) {
				if (!pending.at(i)) continue;

				bool res = true;

				if (live.at(i)) {
					res = recreate_table[i](*this);
				} else {
					create_table[i](*this);
					live.set(i);
				}
				
				if (res) {
					pending |= graph::dependents[i];
				}
			}
		}

		template<typename ... Us>
		void recreate() {
			recreate(graph::mask_of(meta::type_list<Us...>{}));
		}

		template<typename T>
		T& get() {
			return std::get<T>(resources);
		}

		decltype(auto) invoke(auto F) {
			return [&]<typename ... arg_Ts>(meta::type_list<arg_Ts...>) {
				if constexpr (std::is_member_function_pointer_v<decltype(F)>) {
					return std::invoke(F, get<meta::func_class_t<decltype(F)>>(), get<arg_Ts>()...);
				} else {
					return std::invoke(F, get<arg_Ts>()...);
				}
			}(meta::func_args_t<decltype(F)>{});
		}

	private:
        // table of thunks
		static constexpr std::array<void(*)(Registry&), N> create_table = { [](Registry& reg) { return reg.invoke(&Ts::create); }... };
		static constexpr std::array<bool(*)(Registry&), N> recreate_table = { [](Registry& reg) { return reg.invoke(&Ts::recreate); }... };
		static constexpr std::array<void(*)(Registry&), N> destroy_table = { [](Registry& reg) { return reg.invoke(&Ts::destroy); }... };
		
		mask live;
		std::tuple<Ts...> resources;
	};

    namespace detail {
        template<typename T> concept has_resources = requires { typename T::resources; };
        
        template<typename L, typename T> struct contains;
        template<typename ... Ls, typename T> struct contains<meta::type_list<Ls...>, T> : std::bool_constant<(std::is_same_v<Ls, T> || ...)> { };
        
        
        template<typename Seen, typename Pending> struct collect;
        template<typename Seen> struct collect<Seen, meta::type_list<>> { using type = Seen; };
        template<typename Seen, typename T, typename ... Rest> 
        struct collect<Seen, meta::type_list<T, Rest...>> {
            using type = typename std::conditional_t<
                contains<Seen, T>::value, collect<Seen, meta::type_list<Rest...>>,
                collect<meta::concat_t<Seen, meta::type_list<T>>, meta::concat_t<meta::type_list<Rest...>, meta::func_args_t<decltype(&T::create)>>>
            >::type;
        };
        
        
        template<typename L> struct to_registry;
        template<typename ... Ts> struct to_registry<meta::type_list<Ts...>> { using type = Registry<Ts...>; };
        
    }
    
    template<typename ... Ts>
    struct RegistryBuilder {
        using resources = typename detail::collect<meta::type_list<>, meta::concat_t<meta::type_list<Ts>...>>::type;
        using type = typename detail::to_registry<resources>::type;
        template<typename T> static constexpr bool contains = detail::contains<resources, T>::value;
        template<typename ... Us> using with = RegistryBuilder<Ts..., Us...>;
    };

    template<typename ... Ts>
    using reg_builder = typename RegistryBuilder<Ts...>::type;
}