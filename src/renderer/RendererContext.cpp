#include "RendererContext.hpp"

#include "logging_macros.h"

#include <webgpu/webgpu.h>
#include <stdexcept>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>   
#endif  // WEBGPU_BACKEND_WGPU

#include <utility>

// for file reading in loadShaderModule, maybe move to ResourceManager later
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

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
    RDContext& context = *reinterpret_cast<RDContext*>(userdata);
	if (status == WGPURequestAdapterStatus_Success || status == 1) {
        context.adapter = p_adapter;
		LOG_TRACE("  ~  got WebGPU adapter! %p", p_adapter);
        ERR(context.adapter == nullptr, "Adapter is null when requested ended and status is success. Browser is not supported");
	} else {
		ERR(true, "WebGPU could not get adapter: %s", message);
	}
    context.yieldToBrowser = false;
}

void onDeviceRequestEnded(
		WGPURequestDeviceStatus status,
		WGPUDevice device,
		const char* message,
		void* userdata
) {
    RDContext& context = *reinterpret_cast<RDContext*>(userdata);
	if (status == WGPURequestDeviceStatus_Success || status == 1) {
        context.device = device;
		LOG_TRACE("  ~  got WebGPU device!");
	} else {
		ERR(true, "WebGPU could not get device: %s", message);
	}
    context.yieldToBrowser = false;
}

void RDContext::Initialize(WGPUInstance p_instance, RDSurface p_rdSurface) {
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
    yieldToBrowser = true;

#ifdef __EMSCRIPTEN__
	while (yieldToBrowser) {
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

	LOG_INFO("Renderer context initialized");
}

WGPUShaderModule RDContext::LoadShaderModule(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open shader: " + filename);
    }
    LOG_TRACE("Shader file opened: %s", filename.c_str());
    std::ostringstream contents;
    contents << file.rdbuf();
    std::string source = contents.str();

    WGPUShaderModuleWGSLDescriptor shaderDesc = {
        .chain = {
            .next = nullptr,
            .sType = WGPUSType_ShaderModuleWGSLDescriptor,
        },
        .code = source.c_str(),
    };

    WGPUShaderModuleDescriptor moduleDesc = {
        .nextInChain = reinterpret_cast<WGPUChainedStruct*>(&shaderDesc),
        .label = filename.c_str(),
    };

    WGPUShaderModule module = wgpuDeviceCreateShaderModule(device, &moduleDesc);
    LOG_INFO("Shader module loaded: %s", filename.c_str());

    return module;
}

RDContext::RDContext() {
	instance = nullptr;
	adapter = nullptr;
	device = nullptr;
	queue = nullptr;
    yieldToBrowser = false;
}

RDContext::~RDContext() {
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
	if (device) {
		wgpuDeviceRelease(device);
        LOG_TRACE("Device released");
	} else {
		LOG_WARN("Device is null when destroying renderer context");
	}
	if (queue) {
		wgpuQueueRelease(queue);
        LOG_TRACE("Queue released");
	} else {
		LOG_WARN("Queue is null when destroying renderer context");
	}
	LOG_INFO("Renderer context destroyed");
}

void RDContext::ConfigureSurface(const int& width, const int& height) {
    ZoneScoped;
	ERR(adapter == nullptr, "Adapter is null, possibly context is not initialized");
	ERR(device == nullptr, "Device is null, possibly context is not initialized");

	WGPUSurfaceCapabilities capabilities = {};
	wgpuSurfaceGetCapabilities(rdSurface.surface, adapter, &capabilities);
	WGPUSurfaceConfiguration config = {
		.nextInChain = nullptr,
		.device = device,
		.format = capabilities.formats[0],
		.usage = WGPUTextureUsage_RenderAttachment,
		.viewFormatCount = 0,
		.viewFormats = nullptr,
		.alphaMode = WGPUCompositeAlphaMode_Auto,
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
		.presentMode = WGPUPresentMode_Fifo,
	};
	rdSurface.format = capabilities.formats[0];

	wgpuSurfaceConfigure(rdSurface.surface, &config);
	LOG_TRACE("Surface configured");
}

WGPUTextureView RDContext::NextTextureView() {
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

void RDContext::Polltick() {
    ZoneScoped;
    #if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(device, false, nullptr);
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    if (yieldToBrowser) {
        emscripten_sleep(100);
    }
#endif
}
