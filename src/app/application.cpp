#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif	// WEBGPU_BACKEND_WGPU

#include "application.h"
#include "webgpu-utils.h"

#include <iostream>

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
	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOptions);
	std::cout << "Adapter created: " << adapter << std::endl;
	wgpuInstanceRelease(instance);


	std::cout << "Requesting device" << std::endl;
	WGPUDeviceDescriptor deviceDesc = {};

	deviceDesc.label = wgpuStringViewFromCString("My Device");
	deviceDesc.requiredFeatureCount = 0;

	deviceDesc.defaultQueue.label = wgpuStringViewFromCString("My Queue");

	_device = requestDeviceSync(instance, adapter, &deviceDesc);
	wgpuAdapterRelease(adapter);
	std::cout << "Device created: " << _device << std::endl;

	_queue = wgpuDeviceGetQueue(_device);

	std::cout << "Configuring surface" << std::endl;
	WGPUSurfaceConfiguration config = {};
	config.nextInChain = nullptr;
	config.height = height;
	config.width = width;

	WGPUSurfaceCapabilities capabilities = {};
	wgpuSurfaceGetCapabilities(_surface, adapter, &capabilities);
	config.format = capabilities.formats[0];
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.usage = WGPUTextureUsage_RenderAttachment;
	config.device = _device;
	config.presentMode = WGPUPresentMode_Fifo;
	config.alphaMode = WGPUCompositeAlphaMode_Auto;

	wgpuSurfaceConfigure(_surface, &config);
	std::cout << "Surface configured" << std::endl;

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

    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.label = wgpuStringViewFromCString("My Shader");

	return true;
}

void Application::terminate() {
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
	encoderDesc.label = wgpuStringViewFromCString("My Command Encoder");
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_device, &encoderDesc);

	WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.label = wgpuStringViewFromCString("My Render Pass");

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
	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);

	WGPUCommandBufferDescriptor commandBufferDesc = {};
	commandBufferDesc.label = wgpuStringViewFromCString("My Command Buffer");
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
	viewDescriptor.label = wgpuStringViewFromCString("My Texture View");
	viewDescriptor.format = wgpuTextureGetFormat(surface_texture.texture);
	viewDescriptor.dimension = WGPUTextureViewDimension_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = WGPUTextureAspect_All;
	WGPUTextureView target_view = wgpuTextureCreateView(surface_texture.texture, &viewDescriptor);

	wgpuTextureRelease(surface_texture.texture);

	return target_view;
}

bool Application::isRunning() {
	return !glfwWindowShouldClose(_window);
}
