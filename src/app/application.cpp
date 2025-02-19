#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <string.h>
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
        p = vec2f(-0.3, -0.5);
    } else if (in_vertex_index == 1u) {
        p = vec2f(0.5, -0.5);
    } else {
        p = vec2f(0.0, 0.5);
    }
    return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.8, 1.0, 1.0);
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
	WGPURequestAdapterOptions adapterOptions = {};
	adapterOptions.compatibleSurface = _surface;
	_adapter = requestAdapterSync(instance, &adapterOptions);
	std::cout << "Adapter created: " << _adapter << std::endl;

	wgpuInstanceRelease(instance);

	std::cout << "Requesting device" << std::endl;
	WGPUDeviceDescriptor deviceDesc = {};

	deviceDesc.label = { "My Device", WGPU_STRLEN };
	deviceDesc.requiredFeatureCount = 0;

	deviceDesc.defaultQueue.label = { "My Queue", WGPU_STRLEN };

	_device = requestDeviceSync(instance, _adapter, &deviceDesc);
	std::cout << "Device created: " << _device << std::endl;

	_queue = wgpuDeviceGetQueue(_device);

	WGPUSurfaceConfiguration config = {};
	config.nextInChain = nullptr;
	config.height = height;
	config.width = width;

	WGPUSurfaceCapabilities capabilities = {};
	wgpuSurfaceGetCapabilities(_surface, _adapter, &capabilities);
	config.format = capabilities.formats[0];
    _surfaceFormat = config.format;
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.usage = WGPUTextureUsage_RenderAttachment;
	config.device = _device;
	config.presentMode = WGPUPresentMode_Fifo;
	config.alphaMode = WGPUCompositeAlphaMode_Auto;

	wgpuSurfaceConfigure(_surface, &config);
	std::cout << "Surface configured" << std::endl;

	initPipeline();

	std::cout << "Initialize finish" << std::endl;

	return true;
}

void Application::initPipeline() {
	WGPUShaderModuleDescriptor shaderDesc{};

	WGPUShaderSourceWGSL shaderCodeDesc{};
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;

	shaderDesc.nextInChain = &shaderCodeDesc.chain;
	shaderCodeDesc.code = { .data = shaderSource, .length = WGPU_STRLEN };
	WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(_device, &shaderDesc);

	WGPURenderPipelineDescriptor pipelineDesc{};
	pipelineDesc.nextInChain = nullptr;

	pipelineDesc.vertex.bufferCount = 0;
	pipelineDesc.vertex.buffers = nullptr;

	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = { .data = "vs_main", .length = WGPU_STRLEN };
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
	
	pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
	
	pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
	
	pipelineDesc.primitive.cullMode = WGPUCullMode_None;

	WGPUFragmentState fragmentState{};
	fragmentState.module = shaderModule;
	fragmentState.entryPoint = { .data = "fs_main", .length = WGPU_STRLEN };
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;

	WGPUBlendState blendState{};
	blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
	blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
	blendState.color.operation = WGPUBlendOperation_Add;
	blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
	blendState.alpha.dstFactor = WGPUBlendFactor_One;
	blendState.alpha.operation = WGPUBlendOperation_Add;
	
	WGPUColorTargetState colorTarget{};
	colorTarget.format = _surfaceFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.
	
	// We have only one target because our render pass has only one output color
	// attachment.
	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;
	pipelineDesc.fragment = &fragmentState;

	// We do not use stencil/depth testing for now
	pipelineDesc.depthStencil = nullptr;

	// Samples per pixel
	pipelineDesc.multisample.count = 1;

	// Default value for the mask, meaning "all bits on"
	pipelineDesc.multisample.mask = ~0u;

	// Default value as well (irrelevant for count = 1 anyways)
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	pipelineDesc.layout = nullptr;

	_pipeline = wgpuDeviceCreateRenderPipeline(_device, &pipelineDesc);

	// We no longer need to access the shader module
	wgpuShaderModuleRelease(shaderModule);
}

void Application::terminate() {
	wgpuAdapterRelease(_adapter);
	wgpuDeviceRelease(_device);
	wgpuQueueRelease(_queue);
	wgpuSurfaceUnconfigure(_surface);
	wgpuSurfaceRelease(_surface);
	glfwDestroyWindow(_window);
	glfwTerminate();
}

void Application::mainLoop() {
	glfwPollEvents();

	WGPUTextureView texture_view = nextTextureView();
	if (!texture_view) {
		return;
	}

	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.label = { "My Command Encoder", WGPU_STRLEN };
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_device, &encoderDesc);

	WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.label = { "My Render Pass", WGPU_STRLEN };

	WGPURenderPassColorAttachment colorAttachment = {};
	colorAttachment.view = texture_view;
	colorAttachment.resolveTarget = nullptr;
	colorAttachment.loadOp = WGPULoadOp_Clear;
	colorAttachment.storeOp = WGPUStoreOp_Store;
	colorAttachment.clearValue = { 0.5f, 0.2f, 0.3f, 1.0f };
#ifndef WEBGPU_BACKEND_WGPU
	colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &colorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;

	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

    wgpuRenderPassEncoderSetPipeline(renderPass, _pipeline);
    wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);

	WGPUCommandBufferDescriptor commandBufferDesc = {};
	commandBufferDesc.label = { "My Command Buffer", WGPU_STRLEN };
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

	if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
		return nullptr;
	}

	WGPUTextureViewDescriptor viewDescriptor = {};
	viewDescriptor.label = { "My Texture View", WGPU_STRLEN };
	viewDescriptor.format = wgpuTextureGetFormat(surface_texture.texture);
	viewDescriptor.dimension = WGPUTextureViewDimension_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = WGPUTextureAspect_All;
	WGPUTextureView target_view = wgpuTextureCreateView(surface_texture.texture, &viewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
	wgpuTextureRelease(surface_texture.texture);
#endif

	return target_view;
}

bool Application::isRunning() {
	return !glfwWindowShouldClose(_window);
}
