#include "Context.hpp"

#include "logging_macros.h"
#include "tracy/Tracy.hpp"

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

void RdContext::initialize(wgpu::Instance _instance, Surface _surface, wgpu::Device* p_device, wgpu::Queue* p_queue) {
	ZoneScoped;
	this->instance = std::move(_instance);
	this->surface = std::move(_surface);

	// --- Request Adapter ---
	WGPURequestAdapterOptions options = {
		.nextInChain = nullptr,
		.compatibleSurface = *surface.handle,
		.powerPreference = wgpu::PowerPreference::HighPerformance,
		.backendType = wgpu::BackendType::Undefined,
		.forceFallbackAdapter = false,
	};

	LOG_TRACE("WEBGPU adapter requested");
	{
		auto callback = instance->requestAdapter(
				options,
				[this](wgpu::RequestAdapterStatus status, wgpu::Adapter _adapter, const char* message) {
					if (status == wgpu::RequestAdapterStatus::Success) {
						this->adapter = std::move(_adapter);
						LOG_TRACE(" ~ Adapter received");
					} else {
						ERR(true, "Could not get WebGPU adapter: %s", message);
					}
				}
		);

#ifdef __EMSCRIPTEN__
		while (adapter == nullptr) {
			emscripten_sleep(100);
		}
#endif
	}

	// --- Request Device ---
	WGPUDeviceDescriptor deviceDesc = {
        .nextInChain = nullptr,
        .label = "My Device",
        .requiredFeatureCount = 0,
        .requiredFeatures = nullptr,
        .requiredLimits = nullptr,
        .defaultQueue = {
            .nextInChain = nullptr,
            .label = "My Queue",
        },
        .deviceLostCallback = nullptr,
        .deviceLostUserdata = nullptr,
#ifdef WEBGPU_BACKEND_WGPU
        .uncapturedErrorCallbackInfo = {},
#endif	// WEBGPU_BACKEND_WGPU
    };

#ifndef __EMSCRIPTEN__
	deviceDesc.uncapturedErrorCallbackInfo = {};
#endif

	LOG_TRACE("WEBGPU device requested");
	{
		auto callback = adapter->requestDevice(
				deviceDesc,
				[p_device](wgpu::RequestDeviceStatus status, wgpu::Device _device, const char* message) {
					if (status == wgpu::RequestDeviceStatus::Success) {
                        *p_device = _device;
						LOG_TRACE(" ~ Device received");
					} else {
						ERR(true, "Could not get WebGPU device: %s", message);
					}
				}
		);

#ifdef __EMSCRIPTEN__
		while (p_driver->device == nullptr) {
			emscripten_sleep(100);
		}
#endif
	}
	ERR(p_device == nullptr, "Device is null when request ended, probably browser is not supported");

	// --- Retrieve Queue ---
    wgpu::Queue queue = p_device->getQueue();
    *p_queue = queue;
	LOG_TRACE("WebGPU queue created");

	LOG_INFO("Renderer context initialized");
}

void RdContext::configureSurface(const int& width, const int& height, const WGPUDevice& p_device) {
	ZoneScoped;

	wgpu::SurfaceCapabilities capabilities = {};
	surface.handle->getCapabilities(*adapter, &capabilities);

	WGPUSurfaceConfiguration config = {
		.nextInChain = nullptr,
		.device = p_device,
		.format = surface.format = capabilities.formats[0],
		.usage = wgpu::TextureUsage::RenderAttachment,
		.viewFormatCount = 0,
		.viewFormats = nullptr,
		.alphaMode = wgpu::CompositeAlphaMode::Opaque,
		.width = surface.width = static_cast<uint32_t>(width),
		.height = surface.height = static_cast<uint32_t>(height),
		.presentMode = wgpu::PresentMode::Fifo,
	};

	surface.depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
	surface.handle->configure(config);
	LOG_TRACE("Surface configured");
}

wgpu::TextureView RdContext::nextTextureView() {
	ZoneScoped;
	wgpu::SurfaceTexture surfaceTexture = {};
	surface.handle->getCurrentTexture(&surfaceTexture);

	if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
		return nullptr;
	}

	wgpu::Texture tx = surfaceTexture.texture;
	wgpu::TextureView targetView = tx.createView(WGPUTextureViewDescriptor{
			.nextInChain = nullptr,
			.label = "Surface texture view",
			.format = surface.format,
			.dimension = wgpu::TextureViewDimension::_2D,
			.baseMipLevel = 0,
			.mipLevelCount = 1,
			.baseArrayLayer = 0,
			.arrayLayerCount = 1,
			.aspect = wgpu::TextureAspect::All,
	});

#ifndef WEBGPU_BACKEND_WGPU
    tx.release();
#endif

	return targetView;
}

void RdContext::polltick(const wgpu::Device& p_device) {
    (void)p_device;
	ZoneScoped;
#if defined(WEBGPU_BACKEND_DAWN)
	p_device.tick();
#elif defined(WEBGPU_BACKEND_WGPU)
    // for some reason this doesnt work
    // p_device.poll(false);

	wgpuDevicePoll(p_device, false, nullptr); 
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
	if (yieldToBrowser) {
		emscripten_sleep(100);
	}
#endif
}
