#include "renderer_context.h"
#include "logging-macros.h"

#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif  // __EMSCRIPTEN__

static void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata) {
    if (status == WGPURequestAdapterStatus_Success || status == 1) {
        *(WGPUAdapter*)userdata = adapter;
        LOG_TRACE("  ~  got WebGPU adapter!");
    } else {
        ERR(true, "WebGPU could not get adapter: %s", message);
    }
    
}

static void onDeviceRequestEnded(WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata) {
    if (status == WGPURequestDeviceStatus_Success || status == 1) {
        *(WGPUDevice*)userdata = device;
        LOG_TRACE("  ~  got WebGPU device!");
    } else {
        ERR(true, "WebGPU could not get device: %s", message);
    }
}

void RendererContext::initialize(WGPUInstance p_instance, Surface p_surface) {
    m_waitingAsync = false;
    m_instance = p_instance;
    m_surface = p_surface;

    // ~~~~~~~~~ ADAPTER ~~~~~~~~~~
    WGPURequestAdapterOptions options = {
        .nextInChain = nullptr,
        .compatibleSurface = m_surface.surface,
        .powerPreference = WGPUPowerPreference_Undefined,
        .backendType = WGPUBackendType_Undefined,
        .forceFallbackAdapter = false,
    };

    m_adapter = nullptr;
    LOG_TRACE("WEBGPU adapter requested");
    wgpuInstanceRequestAdapter(m_instance, &options, onAdapterRequestEnded, &m_adapter);

#ifdef __EMSCRIPTEN__
    while(m_adapter == nullptr) {
        LOG_TRACE("adapter %p", m_adapter);
        emscripten_sleep(100);
    }
    LOG_TRACE("Adapter is ready in Emscripten");
#endif  // __EMSCRIPTEN__
    
    ERR(m_adapter == nullptr, "Adapter is null when requested ended, probably device is not ticking, or window is not polling");

    
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
#ifndef __EMSCRIPTEN__
        .uncapturedErrorCallbackInfo = {},
#endif  // __EMSCRIPTEN__
    };

    LOG_TRACE("WEBGPU device requested");
    wgpuAdapterRequestDevice(m_adapter, &deviceDesc, onDeviceRequestEnded, &m_device);

#ifdef __EMSCRIPTEN__
    LOG_TRACE("Waiting for device to be ready in Emscripten");
    while(m_device == nullptr) {
        emscripten_sleep(100);
    }
#endif  // __EMSCRIPTEN__

    ERR(m_device == nullptr, "Device is null when requested ended, probably adapter is not ticking, or window is not polling");


    // ~~~~~~~~~ QUEUE ~~~~~~~~~~
    LOG_TRACE("WebGPU queue created");
    m_queue = wgpuDeviceGetQueue(m_device);
    
    // ~~~~~~~~~ SURFACE ~~~~~~~~~~
    configureSurface(p_surface);


    LOG_TRACE("Renderer context initialized");
}

RendererContext::RendererContext() {
    m_instance = nullptr;
    m_adapter = nullptr;
    m_device = nullptr;
    m_queue = nullptr;
}

RendererContext::~RendererContext() {
    if (m_instance) {
        wgpuInstanceRelease(m_instance);
    } else {
        LOG_WARN("Instance is null when destroying renderer context");
    }
    if (m_surface.surface) {
        wgpuSurfaceRelease(m_surface.surface);
    } else {
        LOG_WARN("Surface is null when destroying renderer context");
    }
    if (m_adapter) {
        wgpuAdapterRelease(m_adapter);
    } else {
        LOG_WARN("Adapter is null when destroying renderer context");
    }
    if (m_device) {
        wgpuDeviceRelease(m_device);
    } else {
        LOG_WARN("Device is null when destroying renderer context");
    }
    if (m_queue) {
        wgpuQueueRelease(m_queue);
    } else {
        LOG_WARN("Queue is null when destroying renderer context");
    }
    LOG_INFO("Renderer context destroyed");
}

void RendererContext::configureSurface(Surface& p_surface) {
    ERR(m_adapter == nullptr, "Adapter is null, possibly context is not initialized");
    ERR(m_device == nullptr, "Device is null, possibly context is not initialized");

    WARN_COND(p_surface.width == 0, "Surface width is 0");
    WARN_COND(p_surface.height == 0, "Surface height is 0");

    WGPUSurfaceCapabilities capabilities = {};
	wgpuSurfaceGetCapabilities(p_surface.surface, m_adapter, &capabilities);
    WGPUSurfaceConfiguration config = {
		.nextInChain = nullptr,
		.device = m_device,
		.format = capabilities.formats[0],
		.usage = WGPUTextureUsage_RenderAttachment,
		.viewFormatCount = 0,
		.viewFormats = nullptr,
		.alphaMode = WGPUCompositeAlphaMode_Auto,
		.width = p_surface.width,
		.height = p_surface.height,
		.presentMode = WGPUPresentMode_Fifo,
	};
    p_surface.format = capabilities.formats[0];

    wgpuSurfaceConfigure(p_surface.surface, &config);
    LOG_TRACE("Surface configured");
}
