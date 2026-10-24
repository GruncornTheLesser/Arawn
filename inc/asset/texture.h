#pragma once
#include <cstdint>
#include <span>

namespace arawn::assets {
	struct Texture { 
		struct CreateInfo {
			uint32_t width, height;
			uint8_t depth, channels;
			std::span<uint8_t> pixels;
		};

		uint8_t tileX, tileY;
	};
}