#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <webgpu/webgpu.h>

#include <cstdint>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif	// WEBGPU_BACKEND_WGPU

#include "application.h"
#include "logging-macros.h"

struct Surface;

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


Application::Window Application::createWindow(uint32_t width, uint32_t height, const char* title) {
	glfwInit();
    LOG_INFO("GLFW initialized");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    Window window = {
        .handle = glfwCreateWindow(width, height, title, NULL, NULL),
        .width = width,
        .height = height,
    };

    if (!window.handle) {
        glfwTerminate();
    }

    LOG_TRACE("Window created: %p", (void*)window.handle);
    LOG_TRACE("  ~  size: %d x %d", window.width, window.height);

    return window;
}

bool Application::initialize() {
    u_int32_t width = 800;
    u_int32_t height = 600;
	m_window = createWindow(width, height, "WebGPU");

    WGPUInstance instance = wgpuCreateInstance(nullptr);
    LOG_TRACE("WebGPU instance created");

    RendererContext::Surface surface = {
        .surface = glfwCreateWindowWGPUSurface(instance, m_window.handle),
        .format = WGPUTextureFormat_Undefined, // format is deferred until surface is configured inside context
        .width = width,
        .height = height,
    };

	m_context.initialize(instance, surface); 

	return true;
}

Application::Application() {
    LOG_TRACE("Application created");
}

void Application::terminate() {
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
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_context.getDevice(), &encoderDesc);

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

	// wgpuRenderPassEncoderSetPipeline(renderPass, m_context.getPipeline());
	// wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);

	WGPUCommandBufferDescriptor commandBufferDesc = {
		.nextInChain = nullptr,
		.label = "My Command Buffer",
	};
	WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
	wgpuCommandEncoderRelease(encoder);

	wgpuQueueSubmit(m_context.getQueue(), 1, &commandBuffer);
	wgpuCommandBufferRelease(commandBuffer);

	wgpuTextureViewRelease(texture_view);
#ifndef __EMSCRIPTEN__
	wgpuSurfacePresent(m_context.getSurface());
#endif

#if defined(WEBGPU_BACKEND_DAWN)
	wgpuDeviceTick(m_context.getDevice());
#elif defined(WEBGPU_BACKEND_WGPU)
	wgpuDevicePoll(m_context.getDevice(), false, nullptr);
#endif
}

WGPUTextureView Application::nextTextureView() {

	WGPUSurfaceTexture surface_texture = {};
	wgpuSurfaceGetCurrentTexture(m_context.getSurface(), &surface_texture);

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
	return !glfwWindowShouldClose(m_window.handle);
}

Application::~Application() {
    if (m_window.handle != nullptr) {
        glfwDestroyWindow(m_window.handle);
        LOG_TRACE("Application window destroyed");
    }
}
