#include "renderer_context.h"

#include "logging_macros.h"

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <utility>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif	// __EMSCRIPTEN__

void onAdapterRequestEnded(
		WGPURequestAdapterStatus status,
		WGPUAdapter adapter,
		const char* message,
		void* userdata
) {
    RDContext* context = reinterpret_cast<RDContext*>(userdata);
	if (status == WGPURequestAdapterStatus_Success || status == 1) {
        context->adapter = adapter;
		LOG_TRACE("  ~  got WebGPU adapter!");
	} else {
		ERR(true, "WebGPU could not get adapter: %s", message);
	}
}

void onDeviceRequestEnded(
		WGPURequestDeviceStatus status,
		WGPUDevice device,
		const char* message,
		void* userdata
) {
    RDContext* context = reinterpret_cast<RDContext*>(userdata);
	if (status == WGPURequestDeviceStatus_Success || status == 1) {
        context->device = device;
		LOG_TRACE("  ~  got WebGPU device!");
	} else {
		ERR(true, "WebGPU could not get device: %s", message);
	}
}

void RDContext::initialize(WGPUInstance p_instance, RDSurface p_rdSurface) {
	instance = p_instance;
	rdSurface = std::move(p_rdSurface);

	// ~~~~~~~~~ ADAPTER ~~~~~~~~~~
	WGPURequestAdapterOptions options = {
		.nextInChain = nullptr,
		.compatibleSurface = rdSurface.surface,
		.powerPreference = WGPUPowerPreference_Undefined,
		.backendType = WGPUBackendType_Undefined,
		.forceFallbackAdapter = false,
	};

	adapter = nullptr;
	LOG_TRACE("WEBGPU adapter requested");
	wgpuInstanceRequestAdapter(instance, &options, onAdapterRequestEnded, this);

#ifdef __EMSCRIPTEN__
	while (adapter == nullptr) {
		LOG_TRACE("adapter %p", adapter);
		emscripten_sleep(100);
	}
	LOG_TRACE("Adapter is ready in Emscripten");
#endif	// __EMSCRIPTEN__

	ERR(adapter == nullptr,
		"Adapter is null when requested ended, probably device is not ticking, or window is not polling");

	// ~~~~~~~~~ DEVICE ~~~~~~~~~~
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
    };

#ifndef __EMSCRIPTEN__
        deviceDesc.uncapturedErrorCallbackInfo = {};
#endif	// __EMSCRIPTEN__

	LOG_TRACE("WEBGPU device requested");
	wgpuAdapterRequestDevice(adapter, &deviceDesc, onDeviceRequestEnded, this);

#ifdef __EMSCRIPTEN__
	LOG_TRACE("Waiting for device to be ready in Emscripten");
	while (device == nullptr) {
		emscripten_sleep(100);
	}
#endif	// __EMSCRIPTEN__

	ERR(device == nullptr,
		"Device is null when requested ended, probably adapter is not ticking, or window is not polling");

	// ~~~~~~~~~ QUEUE ~~~~~~~~~~
	LOG_TRACE("WebGPU queue created");
	queue = wgpuDeviceGetQueue(device);

	// ~~~~~~~~~ SURFACE ~~~~~~~~~~
	configureSurface(rdSurface);

	LOG_INFO("Renderer context initialized");
}

RDContext::RDContext() {
	instance = nullptr;
	adapter = nullptr;
	device = nullptr;
	queue = nullptr;
}

RDContext::~RDContext() {
	if (instance) {
		wgpuInstanceRelease(instance);
	} else {
		LOG_WARN("Instance is null when destroying renderer context");
	}
    // Window releases surface
	// if (rdSurface.surface) {
	//     wgpuSurfaceRelease(rdSurface.surface);
	// } else {
	//     LOG_WARN("Surface is null when destroying renderer context");
	// }
	if (adapter) {
		wgpuAdapterRelease(adapter);
	} else {
		LOG_WARN("Adapter is null when destroying renderer context");
	}
	if (device) {
		wgpuDeviceRelease(device);
	} else {
		LOG_WARN("Device is null when destroying renderer context");
	}
	if (queue) {
		wgpuQueueRelease(queue);
	} else {
		LOG_WARN("Queue is null when destroying renderer context");
	}
	LOG_INFO("Renderer context destroyed");
}

void RDContext::configureSurface(RDSurface& p_rdSurface) {
	ERR(adapter == nullptr, "Adapter is null, possibly context is not initialized");
	ERR(device == nullptr, "Device is null, possibly context is not initialized");

	WARN_COND(p_rdSurface.width == 0, "Surface width is 0");
	WARN_COND(p_rdSurface.height == 0, "Surface height is 0");

	WGPUSurfaceCapabilities capabilities = {};
	wgpuSurfaceGetCapabilities(p_rdSurface.surface, adapter, &capabilities);
	WGPUSurfaceConfiguration config = {
		.nextInChain = nullptr,
		.device = device,
		.format = capabilities.formats[0],
		.usage = WGPUTextureUsage_RenderAttachment,
		.viewFormatCount = 0,
		.viewFormats = nullptr,
		.alphaMode = WGPUCompositeAlphaMode_Auto,
		.width = p_rdSurface.width,
		.height = p_rdSurface.height,
		.presentMode = WGPUPresentMode_Fifo,
	};
	p_rdSurface.format = capabilities.formats[0];

	wgpuSurfaceConfigure(p_rdSurface.surface, &config);
	LOG_TRACE("Surface configured");
}

WGPUTextureView RDContext::nextTextureView() {
	WGPUSurfaceTexture surface_texture = {};
	wgpuSurfaceGetCurrentTexture(rdSurface.surface, &surface_texture);

	if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
		return nullptr;
	}

	WGPUTextureViewDescriptor viewDescriptor = {
		.nextInChain = nullptr,
		.label = "Surface texture view",
		.format = wgpuTextureGetFormat(surface_texture.texture),
		.dimension = WGPUTextureViewDimension_2D,
		.baseMipLevel = 0,
		.mipLevelCount = 1,
		.baseArrayLayer = 0,
		.arrayLayerCount = 1,
		.aspect = WGPUTextureAspect_All,
	};
	WGPUTextureView target_view = wgpuTextureCreateView(surface_texture.texture, &viewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
	wgpuTextureRelease(surface_texture.texture);
#endif

	return target_view;
}

void RDContext::polltick() {
    #if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(device, false, nullptr);
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    if (yieldToWebBrowser) {
        emscripten_sleep(100);
    }
#endif
}
