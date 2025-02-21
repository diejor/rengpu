#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif	// WEBGPU_BACKEND_WGPU

#include "application.h"
#include "webgpu-utils.h"

#include <iostream>

const char* shaderSource = R"(

@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
    var p = vec2f(0.0, 0.0);
    if (in_vertex_index == 0u) {
        p = vec2f(-0.5, -0.5);
    } else if (in_vertex_index == 1u) {
        p = vec2f(0.5, -0.5);
    } else {
        p = vec2f(0.0, 0.5);
    }
    return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
}

    )";

bool Application::initialize() {
	glfwInit();
	glfwSetErrorCallback([](int error, const char* description) {
		std::cerr << "GLFW error " << error << ": " << description << std::endl;
	});

	int width = 800;
	int height = 600;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	_window = glfwCreateWindow(width, height, "WebGPU Example", NULL, NULL);

	if (!_window) {
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}

	WGPUInstance instance = wgpuCreateInstance(nullptr);
	_surface = glfwCreateWindowWGPUSurface(instance, _window);

	std::cout << "Requesting adapter" << std::endl;
	WGPURequestAdapterOptions adapterOptions = {
		.nextInChain = nullptr,
		.compatibleSurface = _surface,
		.powerPreference = WGPUPowerPreference_Undefined,
		.backendType = WGPUBackendType_Undefined,
		.forceFallbackAdapter = false,
	};
	WGPUAdapter _adapter = requestAdapterSync(instance, &adapterOptions);
	std::cout << "Adapter created: " << _adapter << std::endl;
	wgpuInstanceRelease(instance);

	std::cout << "Requesting device" << std::endl;
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
        .uncapturedErrorCallbackInfo = {},
    };
	_device = requestDeviceSync(_adapter, &deviceDesc);
	std::cout << "Device created: " << _device << std::endl;

	_queue = wgpuDeviceGetQueue(_device);

	std::cout << "Configuring surface" << std::endl;
	WGPUSurfaceCapabilities capabilities = {};
	wgpuSurfaceGetCapabilities(_surface, _adapter, &capabilities);

	WGPUSurfaceConfiguration config = {
		.nextInChain = nullptr,
		.device = _device,
		.format = capabilities.formats[0],
		.usage = WGPUTextureUsage_RenderAttachment,
		.viewFormatCount = 0,
		.viewFormats = nullptr,
		.alphaMode = WGPUCompositeAlphaMode_Auto,
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
		.presentMode = WGPUPresentMode_Fifo,
	};
	_surfaceFormat = config.format;
	wgpuSurfaceConfigure(_surface, &config);
	std::cout << "Surface configured" << std::endl;

	std::cout << "Creating pipeline" << std::endl;

	WGPUShaderModuleWGSLDescriptor shaderDesc = {
        .chain = {
            .next = nullptr,
            .sType = WGPUSType_ShaderModuleWGSLDescriptor,
        },
        .code = shaderSource,
    };

	WGPUShaderModuleDescriptor moduleDesc = {
		.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&shaderDesc),
		.label = "My Shader Module",
		.hintCount = 0,
		.hints = nullptr,
	};

	WGPUShaderModule module = wgpuDeviceCreateShaderModule(_device, &moduleDesc);

	WGPUBlendState blendState = {
        .color = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_SrcAlpha,
            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
        },
        .alpha = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_Zero,
            .dstFactor = WGPUBlendFactor_One,
        },
    };

	WGPUColorTargetState colorTargetState = {
		.nextInChain = nullptr,
		.format = _surfaceFormat,
		.blend = &blendState,
		.writeMask = WGPUColorWriteMask_All,
	};

	const WGPUFragmentState fragmentState = {
		.nextInChain = nullptr,
		.module = module,
		.entryPoint = "fs_main",
		.constantCount = 0,
		.constants = nullptr,
		.targetCount = 1,
		.targets = &colorTargetState,
	};

	WGPURenderPipelineDescriptor pipelineDesc = {
        .nextInChain = nullptr,
        .label = "My Pipeline",
        .layout = nullptr,
        .vertex = {
            .nextInChain = nullptr,
            .module = module,
            .entryPoint = "vs_main",
            .constantCount = 0,
            .constants = nullptr,
            .bufferCount = 0,
            .buffers = nullptr,
        },
        .primitive = {
            .nextInChain = nullptr,
            .topology = WGPUPrimitiveTopology_TriangleList,
            .stripIndexFormat = WGPUIndexFormat_Undefined,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_None,
        },
        .depthStencil = nullptr,
        .multisample = {
            .nextInChain = nullptr,
            .count = 1,
            .mask = ~0u,
            .alphaToCoverageEnabled = false,
        },
        .fragment = &fragmentState,  
    };

	_pipeline = wgpuDeviceCreateRenderPipeline(_device, &pipelineDesc);
	std::cout << "Pipeline created" << std::endl;

	wgpuShaderModuleRelease(module);

	return true;
}

void Application::terminate() {
	wgpuAdapterRelease(_adapter);
	wgpuDeviceRelease(_device);
	wgpuQueueRelease(_queue);
	wgpuSurfaceUnconfigure(_surface);
	wgpuSurfaceRelease(_surface);
	wgpuRenderPipelineRelease(_pipeline);
	glfwDestroyWindow(_window);
	glfwTerminate();
}

void Application::mainLoop() {
	glfwPollEvents();

	WGPUTextureView texture_view = nextTextureView();
	if (!texture_view) {
		return;
	}

	WGPUCommandEncoderDescriptor encoderDesc = {
        .nextInChain = nullptr,
        .label = "My Encoder",
    };
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_device, &encoderDesc);

	WGPURenderPassColorAttachment colorAttachment = {
        .nextInChain = nullptr,
        .view = texture_view,
        .depthSlice = 0,
        .resolveTarget = nullptr,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = { 0.5f, 0.2f, 0.3f, 1.0f },
    };

#ifndef WEBGPU_BACKEND_WGPU
	colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

	WGPURenderPassDescriptor renderPassDesc = {
        .nextInChain = nullptr,
        .label = "My Render Pass",
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment,
        .depthStencilAttachment = nullptr,
        .occlusionQuerySet = nullptr,
        .timestampWrites = nullptr,
    };

	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

	wgpuRenderPassEncoderSetPipeline(renderPass, _pipeline);
	wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);

	WGPUCommandBufferDescriptor commandBufferDesc = {
        .nextInChain = nullptr,
        .label = "My Command Buffer",
    };
	WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
	wgpuCommandEncoderRelease(encoder);

	wgpuQueueSubmit(_queue, 1, &commandBuffer);
	wgpuCommandBufferRelease(commandBuffer);

	wgpuTextureViewRelease(texture_view);
#ifndef __EMSCRIPTEN__
	wgpuSurfacePresent(_surface);
#endif

#if defined(WEBGPU_BACKEND_DAWN)
	wgpuDeviceTick(_device);
#elif defined(WEBGPU_BACKEND_WGPU)
	wgpuDevicePoll(_device, false, nullptr);
#endif
}

WGPUTextureView Application::nextTextureView() {
	WGPUSurfaceTexture surface_texture = {};
	wgpuSurfaceGetCurrentTexture(_surface, &surface_texture);

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

bool Application::isRunning() {
	return !glfwWindowShouldClose(_window);
}
