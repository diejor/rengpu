#pragma once

#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

class Window;

struct RdContext {
    struct Surface {
        wgpu::raii::Surface handle;
        wgpu::TextureFormat format;
        wgpu::TextureFormat depthTextureFormat;
        uint32_t width;
        uint32_t height;
    };

	void initialize(wgpu::Instance instance, Surface surface, wgpu::Device* p_device, wgpu::Queue* p_queue);
    void configureSurface(const int& p_width, const int& p_height, const WGPUDevice& p_device);
    void polltick(const wgpu::Device& p_device);
    wgpu::TextureView nextTextureView();

	RdContext& operator=(const RdContext&) = delete;
	RdContext& operator=(const RdContext&&) = delete;

    wgpu::raii::Instance instance;
    wgpu::raii::Adapter adapter;
	Surface surface;
    bool yieldToBrowser;
};
