#pragma once

#include <webgpu/webgpu.h>

struct RDSurface {
	WGPUSurface surface;
	WGPUTextureFormat format;
	uint32_t width;
	uint32_t height;

	RDSurface() : surface(nullptr), format(WGPUTextureFormat_Undefined), width(0), height(0) {}
	RDSurface(WGPUSurface s, WGPUTextureFormat f, uint32_t w, uint32_t h) : surface(s), format(f), width(w), height(h) {}

	RDSurface(const RDSurface&) = delete;
	RDSurface& operator=(const RDSurface&) = delete;

	// ~~~~~~~~~~~~~
	// Only allow to move surfaces since WGPUSurface stores a pointer to window resources.
	// ~~~~~~~~~~~~~
	RDSurface(RDSurface&& other) noexcept :
			surface(other.surface), format(other.format), width(other.width), height(other.height) {
		other.surface = nullptr;
	}
	RDSurface& operator=(RDSurface&& other) noexcept {
		if (this != &other) {
			// Optionally release the current surface if owned.
			surface = other.surface;
			format = other.format;
			width = other.width;
			height = other.height;
			other.surface = nullptr;
		}
		return *this;
	}
};
