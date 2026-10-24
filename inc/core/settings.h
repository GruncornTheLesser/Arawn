#pragma once
#include <cstdint>

namespace arawn::core {
	enum class DisplayMode { WINDOWED, FULLSCREEN, EXCLUSIVE };
	enum class BufferingMode { DOUBLE, TRIPLE };
	enum class VsyncMode { DISABLED, ENABLED };
	enum class LowLatencyMode { DISABLED, BALANCED, ENABLED };
	enum class AntiAliasing { DISABLED, MSAA_2, MSAA_4, FXAA_2, FXAA_4 };
	enum class Quality { LOW, MEDIUM, HIGH };
	
	struct Settings {
		const char* title = "arawn";
		struct { uint32_t variant, major, minor, patch; } version;
		const char* gpu = nullptr;
		const char* monitor = nullptr;
		struct { uint32_t x, y; } resolution = { 800, 600 };
		VsyncMode vsync = VsyncMode::ENABLED;
		LowLatencyMode latency = LowLatencyMode::BALANCED;
		AntiAliasing antialiasing = AntiAliasing::MSAA_2;
		BufferingMode buffering = BufferingMode::DOUBLE;
		uint32_t refreshRate = 0; // max
		DisplayMode display = DisplayMode::WINDOWED;

		struct {
			bool enabled = true;
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
		} materials;

		static constexpr uint32_t MaxMaterialDefault = 1 << 16;

		struct {
			uint64_t allocation = 512 * 1024 * 1024;
			uint32_t maxMeshes = 8 * 1024;
			uint32_t maxMeshesPerCluster = 1024;
			uint32_t maxInstances = 4 * 1024 * 1024;
			uint32_t maxLights = 64 * 1024; // 
			uint32_t maxLightPerTile = 64; // 
			uint32_t maxMaterial = 64 * 1024;
			struct { uint8_t vertices, triangles; } meshletSize = { 64, 64 };
			uint32_t tileSize = 32;
			struct { uint8_t x, y; } clusters = { 8, 2 };
		} budget;
	};
}