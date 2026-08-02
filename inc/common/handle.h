#pragma once
#include <cstdint>

namespace arawn {
	template<class T>
	struct Handle;

	struct Tombstone { };

	template<>
	struct Handle<void> {
		friend class Engine;
		uint32_t index : 20, version : 12;

		Handle(Tombstone = {}) : index(0xfffff) { }
	private:
		Handle(uint32_t idx, uint32_t ver) : index(idx), version(ver) { }
	public:
		bool operator==(Handle other) { return index == other.index && version == other.version; }
		bool operator!=(Handle other) { return index != other.index && version != other.version; }

		bool operator==(Tombstone other) { return index == 0xfffff; }
		bool operator!=(Tombstone other) { return index != 0xfffff; }
	};
	
	template<class T>
	struct Handle : Handle<void> {
		friend class Engine;
		Handle(Tombstone = {}) : Handle<void>(Tombstone{}) { }
	private:
		Handle(uint32_t idx, uint32_t ver) : Handle<void>(idx, ver) { }
	};
}