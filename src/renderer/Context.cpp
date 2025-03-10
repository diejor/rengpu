#include "Context.hpp"

#include "logging_macros.h"

#include <webgpu/webgpu.h>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>   
#endif  // WEBGPU_BACKEND_WGPU

#include <utility>


#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif	// __EMSCRIPTEN__

#include "tracy/Tracy.hpp"


static void onAdapterRequestEnded(
		WGPURequestAdapterStatus status,
		WGPUAdapter p_adapter,
		const char* message,
		void* userdata
) {
    ZoneScoped;
    RdContext& context = *reinterpret_cast<RdContext*>(userdata);
	if (status == WGPURequestAdapterStatus_Success || status == 1) {
        context.adapter = p_adapter;
		LOG_TRACE("  ~  got WebGPU adapter! %p", (void*)p_adapter);
        ERR(context.adapter == nullptr, "Adapter is null when requested ended and status is success. Browser is not supported");
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
    ZoneScoped;
    RdDriver& driver = *reinterpret_cast<RdDriver*>(userdata);
	if (status == WGPURequestDeviceStatus_Success || status == 1) {
        driver.device = device;
		LOG_TRACE("  ~  got WebGPU device!");
	} else {
		ERR(true, "WebGPU could not get device: %s", message);
	}
}

// @brief Initialize the renderer context, driver is passed by pointer to initialize
void RdContext::Initialize(WGPUInstance p_instance, RdSurface p_rdSurface, RdDriver* p_driver) {
    ZoneScoped;
	instance = p_instance;

    ERR(p_rdSurface.surface == nullptr, "Surface is null when initializing renderer context");
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
		emscripten_sleep(100);
	}
#endif	// __EMSCRIPTEN__

	ERR(adapter == nullptr,
		"Adapter is null when requested ended, probably browser is not supported");

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
#ifdef WEBGPU_BACKEND_WGPU
        .uncapturedErrorCallbackInfo = {},
#endif  // WEBGPU_BACKEND_WGPU
    };

#ifndef __EMSCRIPTEN__
        deviceDesc.uncapturedErrorCallbackInfo = {};
#endif	// __EMSCRIPTEN__

	LOG_TRACE("WEBGPU device requested");
	wgpuAdapterRequestDevice(adapter, &deviceDesc, onDeviceRequestEnded, p_driver);

#ifdef __EMSCRIPTEN__
	LOG_TRACE("Waiting for device to be ready in Emscripten");
	while (p_driver->device == nullptr) {
		emscripten_sleep(100);
	}
#endif	// __EMSCRIPTEN__

	ERR(p_driver->device == nullptr,
		"Device is null when requested ended, probably browser is not supported");

	// ~~~~~~~~~ QUEUE ~~~~~~~~~~
	p_driver->queue = wgpuDeviceGetQueue(p_driver->device);
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
		wgpuInstanceRelease(instance);
        LOG_TRACE("Instance released");
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
        LOG_TRACE("Adapter released");
	} else {
		LOG_WARN("Adapter is null when destroying renderer context");
	}
	LOG_INFO("Renderer context destroyed");
}

void RdContext::ConfigureSurface(const int& width, const int& height, const WGPUDevice& p_device) {
    ZoneScoped;
	ERR(adapter == nullptr, "Adapter is null, possibly context is not initialized");

	WGPUSurfaceCapabilities capabilities = {};
	wgpuSurfaceGetCapabilities(rdSurface.surface, adapter, &capabilities);
	WGPUSurfaceConfiguration config = {
		.nextInChain = nullptr,
		.device = p_device,
		.format = rdSurface.format = capabilities.formats[0],
		.usage = WGPUTextureUsage_RenderAttachment,
		.viewFormatCount = 0,
		.viewFormats = nullptr,
		.alphaMode = WGPUCompositeAlphaMode_Auto,
		.width = rdSurface.width = static_cast<uint32_t>(width),
		.height = rdSurface.height = static_cast<uint32_t>(height),
		.presentMode = WGPUPresentMode_Fifo,
	};

    rdSurface.depthTextureFormat = WGPUTextureFormat_Depth24Plus;

	wgpuSurfaceConfigure(rdSurface.surface, &config);
	LOG_TRACE("Surface configured");
}

WGPUTextureView RdContext::NextTextureView() {
    ZoneScoped;
	WGPUSurfaceTexture surfaceTexture = {};
	wgpuSurfaceGetCurrentTexture(rdSurface.surface, &surfaceTexture);

	if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
		return nullptr;
	}

	WGPUTextureViewDescriptor viewDescriptor = {
		.nextInChain = nullptr,
		.label = "Surface texture view",
		.format = wgpuTextureGetFormat(surfaceTexture.texture),
		.dimension = WGPUTextureViewDimension_2D,
		.baseMipLevel = 0,
		.mipLevelCount = 1,
		.baseArrayLayer = 0,
		.arrayLayerCount = 1,
		.aspect = WGPUTextureAspect_All,
	};

    rdSurface.depthTextureFormat = WGPUTextureFormat_Depth24Plus;

	WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
	wgpuTextureRelease(surfaceTexture.texture);
#endif

	return targetView;
}


void RdContext::Polltick(const WGPUDevice& p_device) {
    (void)p_device;
    ZoneScoped;
    #if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(p_device);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(p_device, false, nullptr);
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    if (yieldToBrowser) {
        emscripten_sleep(100);
    }
#endif
}
