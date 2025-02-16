#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu-utils.h>
#include <webgpu/webgpu.h>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif	// WEBGPU_BACKEND_WGPU

#include "application.h"

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
	adapterOptions.nextInChain = nullptr;
    adapterOptions.compatibleSurface = _surface;
	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOptions);
	std::cout << "Adapter created: " << adapter << std::endl;

	wgpuInstanceRelease(instance);

    std::cout << "Requesting device" << std::endl;
	WGPUDeviceDescriptor deviceDesc = {};
	deviceDesc.label = "My Device";
	deviceDesc.nextInChain = nullptr;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.requiredFeatureCount = 0;

	deviceDesc.deviceLostCallbackInfo.nextInChain = nullptr;
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
	deviceDesc.deviceLostCallbackInfo.callback =
			[](const WGPUDevice* device, WGPUDeviceLostReason reason, const char* message, void*) {
				std::cout << "Device lost: " << device << ", reason: " << reason;
				if (message != nullptr) {
					std::cout << ", message: " << message;
				};
				std::cout << std::endl;
			};
	deviceDesc.deviceLostCallbackInfo.userdata = nullptr;

	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "My Queue";

	WGPUUncapturedErrorCallbackInfo uncapturedErrorCallbackInfo = {};
	uncapturedErrorCallbackInfo.nextInChain = nullptr;
	uncapturedErrorCallbackInfo.userdata = nullptr;
	uncapturedErrorCallbackInfo.callback = [](WGPUErrorType type, const char* message, void*) {
		std::cout << "Uncaptured error: type " << type;
		if (message != nullptr) {
			std::cout << ", message: " << message;
		};
		std::cout << std::endl;
	};
    deviceDesc.uncapturedErrorCallbackInfo = uncapturedErrorCallbackInfo;

	_device = requestDeviceSync(adapter, &deviceDesc);
	std::cout << "Device created: " << _device << std::endl;

	_queue = wgpuDeviceGetQueue(_device);

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


	wgpuAdapterRelease(adapter);

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

void Application::main_loop() {
	glfwPollEvents();

	WGPUTextureView texture_view = next_texture_view();
	if (!texture_view) {
		return;
	}

	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = "My Encoder";
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_device, &encoderDesc);

	WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.nextInChain = nullptr;
	renderPassDesc.label = "My Render Pass";


	WGPURenderPassColorAttachment colorAttachment = {};
	colorAttachment.view = texture_view;
	colorAttachment.resolveTarget = nullptr;
	colorAttachment.loadOp = WGPULoadOp_Clear;
	colorAttachment.storeOp = WGPUStoreOp_Store;
	colorAttachment.clearValue = { 0.1f, 0.2f, 0.3f, 1.0f };
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
	commandBufferDesc.nextInChain = nullptr;
	commandBufferDesc.label = "My Command Buffer";
	WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
	wgpuCommandEncoderRelease(encoder);

	wgpuQueueSubmit(_queue, 1, &commandBuffer);
	wgpuCommandBufferRelease(commandBuffer);

	wgpuTextureViewRelease(texture_view);
#ifdef __EMSCRIPTEN__
	wgpuSurfacePresent(_surface);
#endif

#if defined(WEBGPU_BACKEND_DAWN)
	wgpuDeviceTick(_device);
#elif defined(WEBGPU_BACKEND_WGPU)
	wgpuDevicePoll(_device, false, nullptr);
#endif
}

WGPUTextureView Application::next_texture_view() {
	WGPUSurfaceTexture surface_texture = {};
	wgpuSurfaceGetCurrentTexture(_surface, &surface_texture);

	if (surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
		return nullptr;
	}

	WGPUTextureViewDescriptor viewDescriptor = {};
	viewDescriptor.nextInChain = nullptr;
	viewDescriptor.label = "Surface texture view";
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

bool Application::is_running() {
	return !glfwWindowShouldClose(_window);
}
