#pragma once

#include <webgpu/webgpu.h>

struct RdSurface {
	WGPUSurface surface;
	WGPUTextureFormat format;

	RdSurface() : surface(nullptr), format(WGPUTextureFormat_Undefined) {}
	RdSurface(WGPUSurface s, WGPUTextureFormat f) : surface(s), format(f) {}

	RdSurface(const RdSurface&) = delete;
	RdSurface& operator=(const RdSurface&) = delete;

	// ~~~~~~~~~~~~~
	// Only allow to move surfaces since WGPUSurface stores a pointer to window resources.
	// ~~~~~~~~~~~~~
	RdSurface(RdSurface&& other) noexcept :
			surface(other.surface), format(other.format) {
		other.surface = nullptr;
	}
	RdSurface& operator=(RdSurface&& other) noexcept {
		if (this != &other) {
			// Optionally release the current surface if owned.
			surface = other.surface;
			format = other.format;
			other.surface = nullptr;
		}
		return *this;
	}
};
