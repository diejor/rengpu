#pragma once

#include "renderer_surface.h"
#include <webgpu/webgpu.h>
#include <string>

class Window;

struct RDContext {
	void initialize(WGPUInstance p_instance, RDSurface p_rdSurface);
    void configureSurface(int width, int height);
    void polltick();
    WGPUShaderModule loadShaderModule(const std::string& filename);
    WGPUTextureView nextTextureView();

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
