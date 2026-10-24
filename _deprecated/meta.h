#pragma once
#include <stdexcept>
#include <type_traits>
#include <cstdint>
#include <array>
#include <functional>


namespace arawn::meta {
	template<typename ... Ts> struct type_list {};
	
	template<typename T> struct decay_list;
	template<typename ... Ts> struct decay_list<meta::type_list<Ts...>> { using type = meta::type_list<std::decay_t<Ts>...>; };
	template<typename T> using decay_list_t = typename decay_list<T>::type;

	template<typename F> struct func_args : std::type_identity<void> {};
	template<typename F> requires (requires { &F::operator(); }) struct func_args<F> : func_args<decltype(&F::operator())> { };
	template<typename ret_T, typename T, typename ... arg_Ts> struct func_args<ret_T(T::*)(arg_Ts...)>                { using type = type_list<arg_Ts...>; };
	template<typename ret_T, typename T, typename ... arg_Ts> struct func_args<ret_T(T::*)(arg_Ts...) noexcept>       { using type = type_list<arg_Ts...>; };
	template<typename ret_T, typename T, typename ... arg_Ts> struct func_args<ret_T(T::*)(arg_Ts...) const>          { using type = type_list<arg_Ts...>; };
	template<typename ret_T, typename T, typename ... arg_Ts> struct func_args<ret_T(T::*)(arg_Ts...) const noexcept> { using type = type_list<arg_Ts...>; };
	template<typename ret_T, typename ... arg_Ts>             struct func_args<ret_T(*)(arg_Ts...)>                   { using type = type_list<arg_Ts...>; };
	template<typename ret_T, typename ... arg_Ts>             struct func_args<ret_T(*)(arg_Ts...) noexcept>          { using type = type_list<arg_Ts...>; };
	template<typename F> using func_args_t = typename func_args<F>::type;

	template<typename F> struct func_class : std::type_identity<void> {};
	template<typename ret_T, typename T, typename ... arg_Ts> struct func_class<ret_T(T::*)(arg_Ts...)>                { using type = T; };
	template<typename ret_T, typename T, typename ... arg_Ts> struct func_class<ret_T(T::*)(arg_Ts...) noexcept>       { using type = T; };
	template<typename ret_T, typename T, typename ... arg_Ts> struct func_class<ret_T(T::*)(arg_Ts...) const>          { using type = const T; };
	template<typename ret_T, typename T, typename ... arg_Ts> struct func_class<ret_T(T::*)(arg_Ts...) const noexcept> { using type = const T; };
	template<typename F> using func_class_t = typename func_class<F>::type;

	template<typename ... Ts> struct concat;
	template<> struct concat<> { using type = meta::type_list<>; };
	template<typename T> struct concat<T> { using type = T; };
	template<typename ... Ts, typename ... Us, typename ... Vs> struct concat<type_list<Ts...>, type_list<Us...>, Vs...> : concat<type_list<Ts..., Us...>, Vs...> { };
	template<typename ... Ts> using concat_t = typename concat<Ts...>::type;

	template<typename T> struct type_list_size;
	template<typename ... Ts> struct type_list_size<type_list<Ts...>> { static constexpr std::size_t value = sizeof...(Ts); };
	template<typename ... Ts> static constexpr std::size_t type_list_size_v = type_list_size<Ts...>::value;

	template<std::size_t I, typename T> struct at;
	template<std::size_t I, typename T, typename ... Ts> struct at<I, type_list<T, Ts...>> : at<I - 1, type_list<Ts...>> { };
	template<typename T, typename ... Ts> struct at<0, type_list<T, Ts...>> : std::type_identity<T> { };
	template<std::size_t I, typename T> using at_t = at<I, T>::type;

	template<typename T, typename ... Ts> static constexpr std::size_t index_of() {
		if (std::size_t i = -1; ((++i, std::is_same_v<T, Ts>) || ...)) return i;
		return -1;
	}

	template<typename L, typename T> struct contains;
	template<typename ... Ts, typename T> struct contains<type_list<Ts...>, T> : std::bool_constant<(std::is_same_v<Ts, T> || ...)> { };
	template<typename L, typename T> static constexpr bool contains_v = contains<L, T>::value;
}


namespace arawn::engine {
	template<typename T>
	struct get_deps {
		using create_deps = decltype([]{
			if constexpr (requires { requires std::is_member_function_pointer_v<decltype(&T::create)>; })
				return meta::decay_list_t<meta::func_args_t<decltype(&T::create)>>{};
			else
				return meta::type_list<>{};
		}());
		using submit_deps = decltype([]{
			if constexpr (requires { requires std::is_member_function_pointer_v<decltype(&T::submit)>; })
				return meta::decay_list_t<meta::func_args_t<decltype(&T::submit)>>{};
			else
				return meta::type_list<>{};
		}());
		using advance_deps = decltype([]{
			if constexpr (requires { requires std::is_member_function_pointer_v<decltype(&T::advance)>; })
				return meta::decay_list_t<meta::func_args_t<decltype(&T::advance)>>{};
			else
				return meta::type_list<>{};
		}());
		using process_deps = decltype([]{
			if constexpr (requires { requires std::is_member_function_pointer_v<decltype(&T::process)>; })
				return meta::decay_list_t<meta::func_args_t<decltype(&T::process)>>{};
			else
				return meta::type_list<>{};
		}());

		using type = meta::concat_t<create_deps, submit_deps, advance_deps, process_deps>;
	};
	template<typename T>
	using get_deps_t = typename get_deps<T>::type;

    template<typename ... Ts> struct engine;

    namespace detail {
        template<typename Out, typename In> struct collect;

        template<typename Out> struct collect<Out, meta::type_list<>> { using type = Out; };

        template<typename Out, typename In, typename ... Ins>
        struct collect<Out, meta::type_list<In, Ins...>> {
            static constexpr bool contains = meta::contains<Out, In>::value;
            using type = typename collect<std::conditional_t<contains, Out, meta::concat_t<Out, meta::type_list<In>>>,
                std::conditional_t<contains, meta::type_list<Ins...>, meta::concat_t<meta::type_list<Ins...>, get_deps_t<In>>>
            >::type;
        };

        template<typename L> struct to_engine;
        template<typename ... Ts>
        struct to_engine<meta::type_list<Ts...>> { using type = engine<Ts...>; };
    }

    template<typename ... Ts>
    struct builder {
        using resources = typename detail::collect<meta::type_list<>, meta::type_list<Ts...>>::type;
        using type = typename detail::to_engine<resources>::type;

        template<typename ... Us> using with = builder<Ts..., Us...>;
    };

    template<typename ... Ts>
    using builder_t = typename builder<Ts...>::type;
}


namespace arawn::engine {
	template<std::size_t N>
	struct bitmask {
			static_assert(N > 0);
			static constexpr std::size_t W = 64, M = (N + W - 1) / W;

			constexpr bitmask() { }
			constexpr operator bool() const { return any(); }

			constexpr void set(std::size_t n, bool v = true) { const uint64_t m = uint64_t{1} << (n % W); if (v) bits[n / W] |= m; else bits[n / W] &= ~m; }
			constexpr bool at(std::size_t n) const { return (bits[n / W] >> (n % W)) & 1; }
			constexpr bool any() const { for (auto b : bits) if (b) return true; return false; }
			
			constexpr bitmask& operator|=(const bitmask& o) { for (std::size_t i = 0; i < M; ++i) bits[i] |= o.bits[i]; return *this; }
			constexpr bitmask operator|(const bitmask& o) const { bitmask val = *this; val |= o; return val; }
			
			constexpr bitmask& operator&=(const bitmask& o) { for (std::size_t i = 0; i < M; ++i) bits[i] &= o.bits[i]; return *this; }
			constexpr bitmask operator&(const bitmask& o) const { bitmask val = *this; val &= o; return val; }

			constexpr bool subset_of(const bitmask& o) const { for (std::size_t i = 0; i < M; ++i) if (bits[i] & ~o.bits[i]) return false; return true; }
		
			std::array<uint64_t, M> bits{};
	};
}

namespace arawn::engine {
	template<std::size_t N, std::size_t M> // tasks
	struct graph {
		static constexpr std::size_t resource_count = N;
		static constexpr std::size_t task_count = M;

		using resource_mask = bitmask<resource_count>;
		using task_mask = bitmask<task_count>;
		
		struct task {
			resource_mask read;
			resource_mask write;
			task_mask dependencies;
			task_mask dependents;
		};
		
		template<typename ... Ts, typename ... Fs, typename D> requires (sizeof...(Ts) == N && sizeof...(Fs) == M)
		constexpr graph(meta::type_list<Ts...>, meta::type_list<Fs...>, D&& is_dependent) {
			// construct read matrix
			tasks = {
				[]<typename ... arg_Ts>(meta::type_list<arg_Ts...>) { 
					return task{
						.read = []{
							if constexpr (requires { typename Fs::read_set; }) {
								return typename Fs::read_set{};
							} else {
								return meta::concat_t<
									// if base class is const add to read mask
									std::conditional_t<std::is_member_function_pointer_v<Ts> && std::is_const_v<meta::func_class_t<Ts>>,
										meta::type_list<meta::func_class_t<Ts>>, meta::type_list<>
									>, 
									// if arg is not const or not ref add to read mask
									std::conditional_t<!std::is_reference_v<arg_Ts> || std::is_const_v<std::remove_reference_t<arg_Ts>>,
										meta::type_list<std::decay_t<arg_Ts>>, meta::type_list<>
									>... 
								>{};
							}
						}(),
						.write = []{
							if constexpr (requires { typename Fs::write_set; }) {
								return typename Fs::write_set{};
							} else {
								return meta::concat_t<
									// if base class is not const add to write mask
									std::conditional_t<std::is_member_function_pointer_v<Ts> && !std::is_const_v<meta::func_class_t<Ts>>, 
										meta::type_list<meta::func_class_t<Ts>>, meta::type_list<>
									>, 
									// if arg is no const ref add to write mask
									std::conditional_t<std::is_reference_v<arg_Ts> && !std::is_const_v<std::remove_reference_t<arg_Ts>>,
										meta::type_list<std::decay_t<arg_Ts>>, meta::type_list<>
									>...
								>{};
							}
						}(),
					};
				}(meta::func_args_t<Ts>{})...
			};
		
			for (std::size_t i = 0; i < M; ++i) {
				for (std::size_t j = 0; j < M; ++j) {
					if (i == j) continue; 

					if (is_dependent(tasks[j], tasks[i])) {
						tasks[i].dependencies.set(j);
						tasks[j].dependents.set(i);
					}
				}
			}

			task_mask done;

			for (std::size_t k = 0; k < M; ++k) {
				bool found = false;
				for (std::size_t i = 0; i < M && !found; ++i) {
					if (done.at(i)) continue;
					bool ready = true;
					
					for (std::size_t j = 0; j < M; ++j) {
						if (tasks[i].dependencies.at(j) && !done.at(j)) {
							ready = false;
						}
					}
					
					if (ready) {
						done.set(i);
						topology[k] = i;
						found = true;
					}
				}
				if (!found) throw std::logic_error("pass dependency cycle");
			}
		}
		constexpr std::size_t operator[](std::size_t i) const { return topology[i]; }
		constexpr std::size_t size() const { return topology.size(); }


		std::array<task, M> tasks{};
		std::array<std::size_t, M> topology{};
	};

	template<typename... Ts, typename... Fs, typename D>
	graph(meta::type_list<Ts...>, meta::type_list<Fs...>, D&& d) -> graph<sizeof...(Ts), sizeof...(Fs)>;

	template<typename ... Ts>
	struct engine {
		static constexpr std::size_t N = sizeof...(Ts);	
		struct resource_mask : bitmask<N> {
			constexpr resource_mask() : bitmask<N>() { }
			
			template<typename ... Us>
			constexpr resource_mask(meta::type_list<Us...>) {
				(bitmask<N>::set(meta::index_of<Us, Ts...>(), true), ...);
			}
		};
		
		using resource_set = meta::type_list<Ts...>;
		using construction_set = meta::concat_t<std::conditional_t<requires { &Ts::create; }, meta::type_list<Ts>, meta::type_list<>>...>;
		using submission_set = meta::concat_t<std::conditional_t<requires { &Ts::submit; }, meta::type_list<Ts>, meta::type_list<>>...>;

		static constexpr auto construction_graph = graph(resource_set{},
			[]<typename ... Us>(meta::type_list<Us...>){ return meta::type_list<decltype(&Us::create)...>{}; }(construction_set{}),
			[](const auto& lhs, const auto& rhs) -> bool { return (lhs.write & rhs.read).any(); }
		);

		template<typename ... Us>
		explicit engine(Us&& ... rs) : resources() {
			// seed parameters into resources
			((get<Us>() = std::move(rs)), ...);

			// invoke create on each resource in the sequence of construction graph
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				([&]<std::size_t I>(std::integral_constant<std::size_t, I>){
					using T = meta::at_t<I, construction_set>;
					invoke(&T::create);
				}(std::integral_constant<std::size_t, construction_graph[Is]>{}), ...);
			}(std::make_index_sequence<construction_graph.size()>{});
		}
		~engine() {
			// invoke destroy on each resource in reverse sequence of construction graph
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				([&]<std::size_t I>(std::integral_constant<std::size_t, I>){
					using T = meta::at_t<I, construction_set>;
					if constexpr (requires { &T::destroy; }) {
						invoke(&T::destroy);
					}
				}(std::integral_constant<std::size_t, construction_graph[Is]>{}), ...);
			}(std::make_index_sequence<construction_graph.size()>{});
		}

		void render() {

		}
		
	private:
		template<typename T>
		T& get() {
			return std::get<std::decay_t<T>>(resources);
		}

		template<typename T>
		decltype(auto) invoke(T&& f) {
			return [&]<typename ... arg_Ts>(meta::type_list<arg_Ts...>) {
				if constexpr (std::is_member_function_pointer_v<T>) {
					return std::invoke(f, get<meta::func_class_t<T>>(), get<arg_Ts>()...);
				} else {
					return std::invoke(f, get<arg_Ts>()...);
				}
			}(meta::func_args_t<T>{});
		}

		template<typename ... Fs> requires (sizeof...(Fs) > 1)
		decltype(auto) invoke(Fs&& ... fs) {
			static constexpr auto task_graph = graph(resource_set{}, meta::type_list<std::decay_t<Fs>...>{},
				[](const auto& lhs, const auto& rhs) -> bool { return &lhs < &rhs && (lhs.write & rhs.read).any() || (lhs.read & rhs.write).any() || (lhs.write & rhs.write).any(); }
			);
			
			auto funcs = std::forward_as_tuple(std::forward<Fs>(fs)...);

			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				(invoke(std::get<task_graph[Is]>(funcs)), ...);
			}(std::make_index_sequence<task_graph.size()>{});
		}

		void recreate(resource_mask update) {
			resource_mask dirty = update;					// dirty flags downstream recreate candidates 
			resource_mask stale{};							// stale flags destroy + create required(recreate() returns false) 
			
			// propagate dirty
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				([&]<std::size_t I>(std::integral_constant<std::size_t, I>){
					if (!dirty.at(I)) return;
					dirty |= construction_graph.dependents[I];
					
					using T = meta::at_t<I, resource_set>;
					if constexpr (requires { &T::halt; }) {
						invoke(&T::halt);
					}
				}(std::integral_constant<std::size_t, construction_graph[Is]>{}), ...);
			}(std::make_index_sequence<construction_graph.size()>{});

			// determine stale
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				([&]<std::size_t I>(std::integral_constant<std::size_t, I>){
					using T = meta::at_t<I, resource_set>;
					if (dirty.at(I)) return;

					bool propagate = stale.at(I);

					if constexpr (requires { &T::query; }) {
						if (!propagate) {
							propagate |= invoke(&T::query);
						}
					}

					if (propagate) {
						stale |= construction_graph.dependents[I];
					}

				}(std::integral_constant<std::size_t, construction_graph[Is]>{}), ...);
			}(std::make_index_sequence<construction_graph.size()>{});

			// destroy stale
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				([&]<std::size_t I>(std::integral_constant<std::size_t, I>){
					using T = meta::at_t<I, resource_set>;
					if (!stale.at(I)) return;

					if constexpr (requires { &T::destroy; }) {
						invoke(&T::destroy);
					}
				}(std::integral_constant<std::size_t, construction_graph[construction_graph.size() - Is - 1]>{}), ...);
			}(std::make_index_sequence<construction_graph.size()>{});
			
			// recreate stale
			[&]<std::size_t ... Is>(std::index_sequence<Is...>) {
				([&]<std::size_t I>(std::integral_constant<std::size_t, I>){
					using T = meta::at_t<I, resource_set>;
					if (stale.at(I)) return;

					if constexpr (requires { &T::create; }) {
						invoke(&T::create);
					}
				}(std::integral_constant<std::size_t, construction_graph[Is]>{}), ...);
			}(std::make_index_sequence<construction_graph.size()>{});
		}

		template<typename ... Us>
		void recreate() {
			return recreate({ meta::type_list<Us...>{} });
		}


		template<typename ... Us>
		void submit(meta::type_list<Us...> = meta::concat_t<std::conditional_t<requires { &Ts::submit; }, meta::type_list<Ts>, meta::type_list<>>...>{}) {
			invoke(&Us::submit...);
		}

		template<typename ... Us>
		void advance(meta::type_list<Us...> = meta::concat_t<std::conditional_t<requires { &Ts::advance; }, meta::type_list<Ts>, meta::type_list<>>...>{}) {
			invoke(&Us::advance...);
		}

		template<typename ... Us>
		void process(meta::type_list<Us...> = meta::concat_t<std::conditional_t<requires { &Ts::process; }, meta::type_list<Ts>, meta::type_list<>>...>{}) {
			invoke(&Us::process...);
		}

		void halt(uint32_t i) {
			// TODO: use create topology
			
		}
		std::tuple<Ts...> resources{};
	};
}