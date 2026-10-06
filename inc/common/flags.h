#pragma once
#include <type_traits>

namespace arawn::common {
	template<class T>
	struct Flags {
		static_assert(std::is_enum_v<T>);
		using integral_type = std::underlying_type_t<T>;

	private:
		constexpr Flags(integral_type val) : data(val) { }
	public:
		constexpr Flags() : data(0) { }
		constexpr Flags(T e) : data(static_cast<integral_type>(e)) { }
		
		template<std::size_t N>
		constexpr Flags(const T (&arr)[N]) : data(0) {
			for (std::size_t i = 0; i < N; ++i) data |= static_cast<integral_type>(arr[i]);
		}
		
		constexpr operator bool() { return data != 0; }
		constexpr operator integral_type() { return data; }

		constexpr bool operator&=(Flags other) { return data &= other.data; }
		constexpr Flags operator&(Flags other) { return { data & other.data }; }
		
		constexpr bool operator|=(Flags other) { return data |= other.data; }
		constexpr Flags operator|(Flags other) { return { data | other.data }; }
	private:
		integral_type data;
	};
}

namespace arawn {
	template <class T> requires (std::is_enum_v<T>)
	constexpr common::Flags<T> operator|(T lhs, T rhs) { return common::Flags<T>(lhs) | common::Flags<T>(rhs); }

	template <class T> requires (std::is_enum_v<T>)
	constexpr common::Flags<T> operator&(T lhs, T rhs) { return common::Flags<T>(lhs) & common::Flags<T>(rhs); }
}