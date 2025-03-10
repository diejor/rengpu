#pragma once

#include "Surface.hpp"
#include "Driver.hpp"
#include <webgpu/webgpu.hpp>

class Window;

struct RdContext {
	void initialize(wgpu::Instance p_instance, RdSurface p_rdSurface, RdDriver* p_driver);
    void configureSurface(const int& p_width, const int& p_height, const WGPUDevice& p_device);
    void polltick(const wgpu::Device& p_device);
    WGPUTextureView nextTextureView();

	RdContext();
	~RdContext();

	RdContext& operator=(const RdContext&) = delete;
	RdContext& operator=(const RdContext&&) = delete;

    wgpu::Instance instance;
	RdSurface rdSurface;
    wgpu::Adapter adapter;
    bool yieldToBrowser;
};
