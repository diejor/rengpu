#pragma once

#include <webgpu/webgpu.h>

struct RDSurface {
	WGPUSurface surface;
	WGPUTextureFormat format;

	RDSurface() : surface(nullptr), format(WGPUTextureFormat_Undefined) {}
	RDSurface(WGPUSurface s, WGPUTextureFormat f) : surface(s), format(f) {}

	RDSurface(const RDSurface&) = delete;
	RDSurface& operator=(const RDSurface&) = delete;

	// ~~~~~~~~~~~~~
	// Only allow to move surfaces since WGPUSurface stores a pointer to window resources.
	// ~~~~~~~~~~~~~
	RDSurface(RDSurface&& other) noexcept :
			surface(other.surface), format(other.format) {
		other.surface = nullptr;
	}
	RDSurface& operator=(RDSurface&& other) noexcept {
		if (this != &other) {
			// Optionally release the current surface if owned.
			surface = other.surface;
			format = other.format;
			other.surface = nullptr;
		}
		return *this;
	}
};
