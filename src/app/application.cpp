#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <webgpu/webgpu.h>

#include <cstdint>
#include <utility>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif	// WEBGPU_BACKEND_WGPU

#include "application.h"
#include "logging_macros.h"

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

    RDSurface rdSurface(
            glfwCreateWindowWGPUSurface(instance, m_window.handle),
            WGPUTextureFormat_Undefined, // format is deferred until surface is configured inside context
            width,
            height
    );

	m_context.initialize(instance, std::move(rdSurface));

    initPipeline();

    LOG_INFO("Application initialized");
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

	WGPUTextureView texture_view = m_context.nextTextureView();
	if (!texture_view) {
		return;
	}

	WGPUCommandEncoderDescriptor encoderDesc = {
		.nextInChain = nullptr,
		.label = "My Encoder",
	};
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_context.device, &encoderDesc);

	WGPURenderPassColorAttachment colorAttachment = {
		.nextInChain = nullptr,
		.view = texture_view,
		.depthSlice = 0,
		.resolveTarget = nullptr,
		.loadOp = WGPULoadOp_Clear,
		.storeOp = WGPUStoreOp_Store,
		.clearValue = { 0.1f, 0.1f, 0.1f, 1.0f },
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

	wgpuRenderPassEncoderSetPipeline(renderPass, m_pipeline);
	wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);

	WGPUCommandBufferDescriptor commandBufferDesc = {
		.nextInChain = nullptr,
		.label = "My Command Buffer",
	};
	WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
	wgpuCommandEncoderRelease(encoder);

	wgpuQueueSubmit(m_context.queue, 1, &commandBuffer);
	wgpuCommandBufferRelease(commandBuffer);

	wgpuTextureViewRelease(texture_view);
#ifndef __EMSCRIPTEN__
	wgpuSurfacePresent(m_context.rdSurface.surface);
#endif

    m_context.polltick();
}

bool Application::isRunning() {
	return !glfwWindowShouldClose(m_window.handle);
}

void Application::initPipeline() {
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

	WGPUShaderModule module = wgpuDeviceCreateShaderModule(m_context.device, &moduleDesc);

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
		.format = m_context.rdSurface.format,
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

	m_pipeline = wgpuDeviceCreateRenderPipeline(m_context.device, &pipelineDesc);
    LOG_INFO("Pipeline initialized");
}

Application::~Application() {
    if (m_window.handle != nullptr) {
        glfwDestroyWindow(m_window.handle);
        LOG_INFO("Application window destroyed");
    }
}
