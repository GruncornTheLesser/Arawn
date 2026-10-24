#pragma once
#include <common/fwd.h>
#include <cstdint>

namespace arawn::common {
	template<>
	struct Handle<void> {
		using version_type = ARAWN_VERSION_TYPE;
		using index_type = ARAWN_INDEX_TYPE;

		static constexpr struct null_type { } null{ };
		
		index_type index;
		version_type version;
		
		Handle() : index(static_cast<index_type>(-1)) { }
		Handle(index_type idx, version_type ver) : index(idx), version(ver) { }
		
		operator uint32_t() const { return index; }

		friend bool operator==(Handle lhs, Handle rhs) { return lhs.index == rhs.index && lhs.version == rhs.version; }
		friend bool operator!=(Handle lhs, Handle rhs) { return lhs.index != rhs.index && lhs.version != rhs.version; }
		friend bool operator==(Handle<void> hnd, null_type other) { return hnd.index == static_cast<index_type>(-1); }
		friend bool operator!=(Handle<void> hnd, null_type other) { return hnd.index != static_cast<index_type>(-1); }
	};
	
	template<class T>
	struct Handle : Handle<void> {
		Handle() : Handle<void>() { }
		Handle(const Handle<void>& hnd) : Handle<void>(hnd) { }
	private:
		Handle(uint32_t idx, uint32_t ver) : Handle<void>(idx, ver) { }
	};
}