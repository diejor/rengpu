#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_wgpu.h"

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <imgui.h>
#include <stdint.h>
#include <sys/types.h>
#include <webgpu/webgpu.h>

#include <cstdint>
#include <utility>
#include <vector>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif	// WEBGPU_BACKEND_WGPU

#include "Application.hpp"
#include "logging_macros.h"
#include "tracy/Tracy.hpp"

void onWindowResize(GLFWwindow* window, int width, int height) {
	auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

	// Call the actual class-member callback
	if (that != nullptr) that->onResize(width, height);
}

Application::Window Application::CreateWindow(int width, int height, const char* title) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	Window window = {
		.handle = glfwCreateWindow(width, height, title, NULL, NULL),
		.width = width,
		.height = height,
	};

	if (!window.handle) {
		glfwTerminate();
	}

	glfwSetWindowUserPointer(window.handle, this);
	glfwSetFramebufferSizeCallback(window.handle, onWindowResize);

	LOG_TRACE("Window created: %p", (void*)window.handle);
	LOG_TRACE("  ~  size: %d x %d", window.width, window.height);

	return window;
}

bool Application::Initialize() {
	int width = 800;
	int height = 600;
	m_window = CreateWindow(width, height, "WebGPU");

	WGPUInstance instance = wgpuCreateInstance(nullptr);
	LOG_TRACE("WebGPU instance created");

	RDSurface rdSurface(
			glfwCreateWindowWGPUSurface(instance, m_window.handle),
			WGPUTextureFormat_Undefined	 // format is deferred until surface is configured
	);

	m_context.Initialize(instance, std::move(rdSurface));
	m_context.ConfigureSurface(width, height);

	InitPipeline();
	InitBuffers();

	if (!InitGui()) {
		return false;
	}

	LOG_INFO("Application initialized");
	return true;
}

bool Application::InitGui() {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOther(m_window.handle, true);
#ifdef __EMSCRIPTEN__
	ImGui_ImplGlfw_InstallEmscriptenCallbacks(m_window.handle, "#canvas");
#endif
	ImGui_ImplWGPU_InitInfo init_info = {};
	init_info.Device = m_context.device;
	init_info.NumFramesInFlight = 3;
	init_info.RenderTargetFormat = m_context.rdSurface.format;
	LOG_WARN("IMGUI: DepthStencilFormat is undefined");
	init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
	ImGui_ImplWGPU_Init(&init_info);

	LOG_INFO("GUI initialized");
	return true;
}

void Application::MainLoop() {
    FrameMark;
    ZoneScoped;
	glfwPollEvents();
	WGPUTextureView texture_view = m_context.NextTextureView();
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

#ifndef EBGPU_BACKEND_WGPU
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
	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, wgpuBufferGetSize(m_vertexBuffer));
	wgpuRenderPassEncoderDraw(renderPass, m_vertexCount, 1, 0, 0);

    UpdateGui();

    // Not sure how to check that FrameBuffer size is valid just in time when imgui has to be rendered.
    // The FrameBuffer size might change in the middle of the frame, after glfwPollEvents() is called.
    // In that case, onResize configured the surface with an old size, and the new size is not yet configured.
    // So, we need to check if the FrameBuffer size is the same as the window size.
    //
    // This is a workaround for the issue, but there should be a way to check if the FrameBuffer size is valid
    // without checking the window size every frame.
    // I tried storing a skipFrame bool in the class and adding two glfwPollEvents() calls, one to configure 
    // the surface when possible, and the other to check if the FrameBuffer size is valid. But it didn't work.
   	int currentWidth, currentHeight;
	glfwGetFramebufferSize(m_window.handle, &currentWidth, &currentHeight);
	if (currentWidth == m_window.width && currentHeight == m_window.height) {
		ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
	}

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

	m_context.Polltick();
}

void Application::onResize(const int& width, const int& height) {
    ZoneScoped;
	LOG_TRACE("Window resized to %d x %d", width, height);

	m_context.ConfigureSurface(width, height);
	m_window.width = width;
	m_window.height = height;
}

void Application::InitPipeline() {
	WGPUShaderModule module = m_context.LoadShaderModule("triangles.wgsl");

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

	WGPUVertexAttribute posAttribute = {
		.format = WGPUVertexFormat_Float32x2,
		.offset = 0,
		.shaderLocation = 0,
	};

	WGPUVertexAttribute colorAttribute = {
		.format = WGPUVertexFormat_Float32x3,
		.offset = 2 * sizeof(float),
		.shaderLocation = 1,
	};

	std::vector<WGPUVertexAttribute> attributes = { posAttribute, colorAttribute };

	WGPUVertexBufferLayout vertexBufferLayout = {
		.arrayStride = 5 * sizeof(float),
		.stepMode = WGPUVertexStepMode_Vertex,
		.attributeCount = 2,
		.attributes = attributes.data(),
	};

	WGPUFragmentState fragmentState = {
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
            .bufferCount = 1,
            .buffers = &vertexBufferLayout,
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

void Application::UpdateGui() {
    ZoneScoped;
	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("Vertex Data")) {
		for (size_t i = 0; i < m_vertexData.size(); i++) {
			char label[64];
			snprintf(label, sizeof(label), "Vertex %zu", i);
			if (ImGui::TreeNode(label)) {
				ImGui::SliderFloat2("Position", &m_vertexData[i].position.x, -1.0f, 1.0f);
				ImGui::ColorPicker3("Color", &m_vertexData[i].color.r);
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();

	// Render ImGui
	ImGui::EndFrame();
	ImGui::Render();

	wgpuQueueWriteBuffer(m_context.queue, m_vertexBuffer, 0, m_vertexData.data(), m_vertexData.size() * sizeof(Vertex));
}

void Application::InitBuffers() {
	// Define your vertex data
	m_vertexData = {
		{ { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },	 { { +0.8f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
		{ { +0.0f, +0.5f }, { 0.0f, 0.0f, 1.0f } },	 { { -0.55f, -0.5f }, { 1.0f, 1.0f, 0.0f } },
		{ { -0.05f, +0.5f }, { 1.0f, 0.0f, 1.0f } }, { { -0.55f, +0.5f }, { 0.0f, 1.0f, 1.0f } },
	};

	m_vertexCount = static_cast<uint32_t>(m_vertexData.size());

	// Create the GPU vertex buffer.
	WGPUBufferDescriptor bufferDesc = {
		.nextInChain = nullptr,
		.label = "My Vertex Buffer",
		.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
		.size = m_vertexCount * sizeof(Vertex),
		.mappedAtCreation = false,
	};

	m_vertexBuffer = wgpuDeviceCreateBuffer(m_context.device, &bufferDesc);

	// Write the vertex data to the buffer.
	wgpuQueueWriteBuffer(m_context.queue, m_vertexBuffer, 0, m_vertexData.data(), bufferDesc.size);

	LOG_INFO("Buffers initialized");
}

Application::Application() {
	LOG_INFO("Application created");
}

bool Application::isRunning() {
	return !glfwWindowShouldClose(m_window.handle);
}

void Application::Terminate() {
	if (m_window.handle != nullptr) {
		glfwDestroyWindow(m_window.handle);
		LOG_TRACE("Application window destroyed");
	}
	if (m_pipeline != nullptr) {
		wgpuRenderPipelineRelease(m_pipeline);
		LOG_TRACE("Pipeline destroyed");
	}
	if (m_vertexBuffer != nullptr) {
		wgpuBufferRelease(m_vertexBuffer);
		LOG_TRACE("Buffers destroyed");
	}
	TerminateGui();
	glfwTerminate();
}

void Application::TerminateGui() {
	ImGui_ImplWGPU_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	LOG_INFO("GUI terminated");
}

Application::~Application() {
	LOG_INFO("Application destroyed");
}
