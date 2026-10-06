#pragma once
#include <common/fwd.h>
#include <cstdint>

namespace arawn::common {
	template<>
	struct Handle<void> {
		friend class Engine;
		uint32_t index : 20, version : 12;

		static constexpr struct Null { } null{};

		Handle() : index(0xfffff) { }
	private:
		Handle(uint32_t idx, uint32_t ver) : index(idx), version(ver) { }
	public:
		friend bool operator==(Handle lhs, Handle rhs) { return lhs.index == rhs.index && lhs.version == rhs.version; }
		friend bool operator!=(Handle lhs, Handle rhs) { return lhs.index != rhs.index && lhs.version != rhs.version; }
		friend bool operator==(Handle<void> hnd, Null other) { return hnd.index == 0xfffff; }
		friend bool operator!=(Handle<void> hnd, Null other) { return hnd.index != 0xfffff; }
	};
	
	template<class T>
	struct Handle : Handle<void> {
		friend class Pool<T>;
		Handle() : Handle<void>() { }
	private:
		Handle(uint32_t idx, uint32_t ver) : Handle<void>(idx, ver) { }
	};
}