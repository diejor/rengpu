#pragma once

#include <webgpu/webgpu.hpp>

struct RdSurface {
    wgpu::Surface surface;
    wgpu::TextureFormat format;
    wgpu::TextureFormat depthTextureFormat;
    uint32_t width;
    uint32_t height;

	RdSurface() : surface(nullptr), format(WGPUTextureFormat_Undefined), depthTextureFormat(WGPUTextureFormat_Undefined), width(0), height(0) {}
	RdSurface(WGPUSurface s) : surface(s), format(WGPUTextureFormat_Undefined), depthTextureFormat(WGPUTextureFormat_Undefined), width(0), height(0) {}

	RdSurface(const RdSurface&) = delete;
	RdSurface& operator=(const RdSurface&) = delete;

	// ~~~~~~~~~~~~~
	// Only allow to move surfaces since WGPUSurface stores a pointer to window resources.
	// ~~~~~~~~~~~~~
	RdSurface(RdSurface&& other) noexcept :
			surface(other.surface), format(other.format), depthTextureFormat(other.depthTextureFormat), width(other.width), height(other.height) {
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
