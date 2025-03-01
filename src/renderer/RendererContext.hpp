#pragma once

#include "RendererSurface.hpp"
#include <webgpu/webgpu.h>
#include <string>

class Window;

struct RDContext {
	void Initialize(WGPUInstance p_instance, RDSurface p_rdSurface);
    void ConfigureSurface(const int& width, const int& height);
    void Polltick();
    WGPUShaderModule LoadShaderModule(const std::string& filename);
    WGPUTextureView NextTextureView();

	RDContext();
	~RDContext();

	RDContext& operator=(const RDContext&) = delete;
	RDContext& operator=(const RDContext&&) = delete;

	WGPUInstance instance;
	RDSurface rdSurface;
	WGPUAdapter adapter;
	WGPUDevice device;
	WGPUQueue queue;
    bool yieldToBrowser;
};
