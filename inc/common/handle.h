#pragma once
#include <common/fwd.h>
#include <cstdint>

namespace arawn::common {
	template<>
	struct Handle<void> {
		static constexpr struct Null { } null{};

		uint32_t index : 20, version : 12;
		
		Handle() : index(0xfffff) { }
		Handle(uint32_t idx, uint32_t ver) : index(idx), version(ver) { }
		
		friend bool operator==(Handle lhs, Handle rhs) { return lhs.index == rhs.index && lhs.version == rhs.version; }
		friend bool operator!=(Handle lhs, Handle rhs) { return lhs.index != rhs.index && lhs.version != rhs.version; }
		friend bool operator==(Handle<void> hnd, Null other) { return hnd.index == 0xfffff; }
		friend bool operator!=(Handle<void> hnd, Null other) { return hnd.index != 0xfffff; }
	};
	
	template<class T>
	struct Handle : Handle<void> {
		Handle() : Handle<void>() { }
		Handle(const Handle<void>& hnd) : Handle<void>(hnd) { }
	private:
		Handle(uint32_t idx, uint32_t ver) : Handle<void>(idx, ver) { }
	};
}