#pragma once

#include "Surface.hpp"
#include "Driver.hpp"
#include <webgpu/webgpu.h>

class Window;

struct RdContext {
	void Initialize(WGPUInstance p_instance, RdSurface p_rdSurface, RdDriver* p_driver);
    void ConfigureSurface(const int& p_width, const int& p_height, const WGPUDevice& p_device);
    void Polltick(const WGPUDevice& p_device);
    WGPUTextureView NextTextureView();

	RdContext();
	~RdContext();

	RdContext& operator=(const RdContext&) = delete;
	RdContext& operator=(const RdContext&&) = delete;

	WGPUInstance instance;
	RdSurface rdSurface;
	WGPUAdapter adapter;
    bool yieldToBrowser;
};
