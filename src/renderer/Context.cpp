#include "Context.hpp"

#include "logging_macros.h"
#include "tracy/Tracy.hpp"

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

void RdContext::initialize(wgpu::Instance p_instance, RdSurface p_rdSurface, RdDriver* p_driver) {
	ZoneScoped;
	instance = p_instance;

	ERR(p_rdSurface.surface == nullptr, "Surface is null when initializing renderer context");
	rdSurface = std::move(p_rdSurface);

	// --- Request Adapter ---
	WGPURequestAdapterOptions options = {
		.nextInChain = nullptr,
		.compatibleSurface = rdSurface.surface,
		.powerPreference = wgpu::PowerPreference::HighPerformance,
		.backendType = wgpu::BackendType::Undefined,
		.forceFallbackAdapter = false,
	};

	adapter = nullptr;
	LOG_TRACE("WEBGPU adapter requested");
	{
		auto callback = instance.requestAdapter(
				options,
				[this](wgpu::RequestAdapterStatus status, wgpu::Adapter p_adapter, const char* message) {
					if (status == wgpu::RequestAdapterStatus::Success) {
						this->adapter = p_adapter;
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
	ERR(adapter == nullptr, "Adapter is null when request ended, probably browser is not supported");

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
		auto callback = adapter.requestDevice(
				deviceDesc,
				[p_driver](wgpu::RequestDeviceStatus status, wgpu::Device p_device, const char* message) {
					if (status == wgpu::RequestDeviceStatus::Success) {
						p_driver->device = p_device;
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
	ERR(p_driver->device == nullptr, "Device is null when request ended, probably browser is not supported");

	// --- Retrieve Queue ---
	p_driver->queue = p_driver->device.getQueue();
	LOG_TRACE("WebGPU queue created");

	LOG_INFO("Renderer context initialized");
}

RdContext::RdContext() {
	ZoneScoped;
	instance = nullptr;
	adapter = nullptr;
	yieldToBrowser = false;
}

RdContext::~RdContext() {
	ZoneScoped;
	if (instance) {
		instance.release();
		LOG_TRACE("Instance released");
	} else {
		LOG_WARN("Instance is null when destroying renderer context");
	}
	// Surface is expected to be released by the window
	if (adapter) {
		adapter.release();
		LOG_TRACE("Adapter released");
	} else {
		LOG_WARN("Adapter is null when destroying renderer context");
	}
	LOG_INFO("Renderer context destroyed");
}

void RdContext::configureSurface(const int& width, const int& height, const WGPUDevice& p_device) {
	ZoneScoped;
	ERR(adapter == nullptr, "Adapter is null, possibly context is not initialized");

	wgpu::SurfaceCapabilities capabilities = {};
	rdSurface.surface.getCapabilities(adapter, &capabilities);

	WGPUSurfaceConfiguration config = {
		.nextInChain = nullptr,
		.device = p_device,
		.format = rdSurface.format = capabilities.formats[0],
		.usage = wgpu::TextureUsage::RenderAttachment,
		.viewFormatCount = 0,
		.viewFormats = nullptr,
		.alphaMode = wgpu::CompositeAlphaMode::Opaque,
		.width = rdSurface.width = static_cast<uint32_t>(width),
		.height = rdSurface.height = static_cast<uint32_t>(height),
		.presentMode = wgpu::PresentMode::Fifo,
	};

	rdSurface.depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
	rdSurface.surface.configure(config);
	LOG_TRACE("Surface configured");
}

WGPUTextureView RdContext::nextTextureView() {
	ZoneScoped;
	wgpu::SurfaceTexture surfaceTexture = {};
	rdSurface.surface.getCurrentTexture(&surfaceTexture);

	if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
		return nullptr;
	}

	wgpu::Texture tx = surfaceTexture.texture;
	wgpu::TextureView targetView = tx.createView(WGPUTextureViewDescriptor{
			.nextInChain = nullptr,
			.label = "Surface texture view",
			.format = rdSurface.format,
			.dimension = wgpu::TextureViewDimension::_2D,
			.baseMipLevel = 0,
			.mipLevelCount = 1,
			.baseArrayLayer = 0,
			.arrayLayerCount = 1,
			.aspect = wgpu::TextureAspect::All,
	});

#ifndef WEBGPU_BACKEND_WGPU
	surfaceTexture.texture.release();
#endif

	return targetView;
}

void RdContext::polltick(const wgpu::Device& p_device) {
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
