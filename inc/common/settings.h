#pragma once
#include <cstdint>

namespace arawn { // settings.h
	enum class DisplayMode { WINDOWED, FULLSCREEN, EXCLUSIVE };
	enum class BufferingMode { DOUBLE, TRIPLE };
	enum class VsyncMode { DISABLED, ENABLED };
	enum class LowLatencyMode { DISABLED, BALANCED, ENABLED };
	enum class AntiAliasing { DISABLED, MSAA_2, MSAA_4, FXAA_2, FXAA_4, TAA };
	enum class Quality { LOW, MEDIUM, HIGH };
	
	struct Settings {
		const char* title = "application";
		struct { uint32_t variant, major, minor, patch; } version;
		const char* gpu = nullptr;
		const char* monitor = nullptr;
		struct { uint32_t x, y; } resolution = { 800, 600 };
		VsyncMode vsync = VsyncMode::ENABLED;
		LowLatencyMode latency = LowLatencyMode::BALANCED;
		BufferingMode buffering = BufferingMode::DOUBLE;
		uint32_t refreshRate = 0; // max
		DisplayMode display = DisplayMode::WINDOWED;

		struct {
			struct { 
				uint32_t width = 256;
				uint32_t height = 256;
			} resolution;
			struct { // this is a bit odd, but width x height images
				uint32_t width = 32;
				uint32_t height = 32; // 1024 textures
			} capacity; // mega texture size
			uint32_t miplevels = 7;
		} texture;

		struct {
			bool maps : 1 = true;
			bool emissive : 1 = true;
			bool normal : 1 = true;
			bool bump : 1 = true;
			bool opacity : 1 = true;
			uint32_t capacity = 16 * 1024; // 16 * 64 * 1024 = 256Kb
		} materials;

		struct { 
			uint64_t budget = 512 * 1024 * 1024;  // 256Mb
			uint32_t meshCount = 1024 * 1024; // 8Mb
			uint32_t instanceCount = 4 * 1024 * 1024; // 32Mb
			struct { uint8_t vertices, triangles; } meshlet = { 64, 64 };
		} geometry;

	};
}