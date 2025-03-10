#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_wgpu.h"
#include "glfw3webgpu.h"

#include <webgpu/webgpu.h>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif	// WEBGPU_BACKEND_WGPU

#include "Application.hpp"
#include "logging_macros.h"
#include "tracy/Tracy.hpp"

#include <vector>

void onWindowResize(GLFWwindow* window, int width, int height) {
	auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

	// Call the actual class-member callback
	if (that != nullptr) that->onResize(width, height);
}

Application::Window Application::CreateWindow(int width, int height, const char* title) {
	ZoneScoped;
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
	ZoneScoped;
	int width = 800;
	int height = 600;
	m_window = CreateWindow(width, height, "WebGPU");

	WGPUInstance instance = wgpuCreateInstance(nullptr);
	LOG_TRACE("WebGPU instance created");

	RdSurface rdSurface(glfwCreateWindowWGPUSurface(instance, m_window.handle));

	m_driver = {
		.device = nullptr,
		.queue = nullptr,
	};

	m_context.Initialize(instance, std::move(rdSurface), &m_driver);
	m_context.ConfigureSurface(width, height, m_driver.device);

	InitBuffers();
	InitPipeline();

	if (!InitGui()) {
		return false;
	}

	LOG_INFO("Application initialized");
	return true;
}

bool Application::InitGui() {
	ZoneScoped;
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
	init_info.Device = m_driver.device;
	init_info.NumFramesInFlight = 3;
	init_info.RenderTargetFormat = m_context.rdSurface.format;
	init_info.DepthStencilFormat = m_context.rdSurface.depthTextureFormat;
	ImGui_ImplWGPU_Init(&init_info);

	LOG_INFO("GUI initialized");
	return true;
}

void Application::MainLoop() {
	FrameMark;
	ZoneScoped;
	glfwPollEvents();
	UpdateGui();

	{
		ZoneScopedN("Update Buffers");
		wgpuQueueWriteBuffer(
				m_driver.queue, m_vertexBuffer, 0, m_vertexData.data(), m_vertexData.size() * sizeof(Vertex)
		);
		float currentTime = static_cast<float>(glfwGetTime());
		wgpuQueueWriteBuffer(m_driver.queue, m_uniformBuffer, 0, &currentTime, sizeof(float));
	}

	WGPUTextureView textureView = m_context.NextTextureView();
	if (!textureView) {
		return;
	}

	WGPUTextureView depthStencilView = m_driver.NextDepthView(m_context.rdSurface);
	if (!depthStencilView) {
		return;
	}

	WGPUCommandEncoderDescriptor encoderDesc = {
		.nextInChain = nullptr,
		.label = "My Encoder",
	};
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_driver.device, &encoderDesc);

	WGPURenderPassColorAttachment colorAttachment = {
		.nextInChain = nullptr,
		.view = textureView,
		.depthSlice = 0,
		.resolveTarget = nullptr,
		.loadOp = WGPULoadOp_Clear,
		.storeOp = WGPUStoreOp_Store,
		.clearValue = { 0.1f, 0.1f, 0.1f, 1.0f },
	};

#ifndef EBGPU_BACKEND_WGPU
	colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

	WGPURenderPassDepthStencilAttachment depthStencilAttachment = {
		.view = depthStencilView,
		.depthLoadOp = WGPULoadOp_Clear,
		.depthStoreOp = WGPUStoreOp_Store,
		.depthClearValue = 1.0f,
		.depthReadOnly = false,
		.stencilLoadOp = WGPULoadOp_Clear,
		.stencilStoreOp = WGPUStoreOp_Store,
		.stencilClearValue = 0,
		.stencilReadOnly = true,
	};

#ifndef WEBGPU_BACKEND_WGPU
    depthStencilAttachment.stencilLoadOp = WGPULoadOp_Undefined;
    depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
#endif

	WGPURenderPassDescriptor renderPassDesc = {
		.nextInChain = nullptr,
		.label = "My Render Pass",
		.colorAttachmentCount = 1,
		.colorAttachments = &colorAttachment,
		.depthStencilAttachment = &depthStencilAttachment,
		.occlusionQuerySet = nullptr,
		.timestampWrites = nullptr,
	};
	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

	{
		ZoneScopedN("Render Pass");

		wgpuRenderPassEncoderSetPipeline(renderPass, m_pipeline);
		wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, m_vertexBuffer, 0, wgpuBufferGetSize(m_vertexBuffer));
		wgpuRenderPassEncoderSetIndexBuffer(
				renderPass, m_indexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(m_indexBuffer)
		);
		wgpuRenderPassEncoderSetBindGroup(renderPass, 0, m_bindGroup, 0, nullptr);
		wgpuRenderPassEncoderDrawIndexed(renderPass, m_indexCount, 1, 0, 0, 0);

		// Not sure how to check that FrameBuffer size is valid just in time when imgui has to be rendered.
		// The FrameBuffer size might change in the middle of the frame, after glfwPollEvents() is called.
		// In that case, onResize configured the surface with an old size, and the new size is not yet configured.
		// So, we need to check if the FrameBuffer size is the same as the window size.
		//
		// This is a workaround for the issue, but there should be a way to check if the FrameBuffer size is valid
		// without checking the window size every frame.
		// I tried storing a skipFrame bool in the class and adding two glfwPollEvents() calls, one to configure
		// the surface when possible, and the other to check if the FrameBuffer size is valid. But it didn't work.
		{
			ZoneScopedN("FrameBuffer size check");
			int currentWidth, currentHeight;
			glfwGetFramebufferSize(m_window.handle, &currentWidth, &currentHeight);
			if (currentWidth == m_window.width && currentHeight == m_window.height) {
				ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
			}
		}

		wgpuRenderPassEncoderEnd(renderPass);
		wgpuRenderPassEncoderRelease(renderPass);

		WGPUCommandBufferDescriptor commandBufferDesc = {
			.nextInChain = nullptr,
			.label = "My Command Buffer",
		};
		WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
		wgpuCommandEncoderRelease(encoder);

		wgpuQueueSubmit(m_driver.queue, 1, &commandBuffer);
		wgpuCommandBufferRelease(commandBuffer);
	}

	wgpuTextureViewRelease(textureView);
	wgpuTextureViewRelease(depthStencilView);
#ifndef __EMSCRIPTEN__
	wgpuSurfacePresent(m_context.rdSurface.surface);
#endif

	m_context.Polltick(m_driver.device);
}

void Application::onResize(const int& width, const int& height) {
	ZoneScoped;
	LOG_TRACE("Window resized to %d x %d", width, height);

	m_context.ConfigureSurface(width, height, m_driver.device);
	m_window.width = width;
	m_window.height = height;
}

void Application::InitPipeline() {
	ZoneScoped;

	m_bindGroupLayout = m_driver.BindGroupLayoutCreate();
	m_bindGroup = m_driver.BindGroupCreate(m_bindGroupLayout, m_uniformBuffer);

	WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {
		.nextInChain = nullptr,
		.label = "My Pipeline Layout",
		.bindGroupLayoutCount = 1,
		.bindGroupLayouts = &m_bindGroupLayout,
	};
	m_pipelineLayout = wgpuDeviceCreatePipelineLayout(m_driver.device, &pipelineLayoutDesc);

	m_pipeline = m_driver.PipelineCreate(m_context.rdSurface, m_pipelineLayout);

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
				ImGui::SliderFloat3("Position", &m_vertexData[i].position.x, -1.0f, 1.0f);
				ImGui::ColorPicker3("Color", &m_vertexData[i].color.r);
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();

	// Render ImGui
	ImGui::EndFrame();
	ImGui::Render();
}

void Application::InitBuffers() {
	ZoneScoped;
	bool loadGeometryQuery = m_driver.GeometryLoad("pyramid.txt", m_vertexData, m_indexData);
	if (!loadGeometryQuery) {
		LOG_ERROR("Failed to load geometry");
	}

	m_indexCount = static_cast<uint32_t>(m_indexData.size());

	WGPUBufferDescriptor vertexBufferDesc = {
		.nextInChain = nullptr,
		.label = "My Vertex Buffer",
		.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
		.size = m_vertexData.size() * sizeof(Vertex),
		.mappedAtCreation = false,
	};
	vertexBufferDesc.size = (vertexBufferDesc.size + 3) & ~3;

	WGPUBufferDescriptor indexBufferDesc = {
		.nextInChain = nullptr,
		.label = "My Index Buffer",
		.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst,
		.size = m_indexData.size() * sizeof(uint16_t),
		.mappedAtCreation = false,
	};
	indexBufferDesc.size = (indexBufferDesc.size + 3) & ~3;

	WGPUBufferDescriptor uniformBufferDesc = {
		.nextInChain = nullptr,
		.label = "My Uniform Buffer",
		.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
		.size = 4 * sizeof(float),
		.mappedAtCreation = false,
	};

	m_vertexBuffer = wgpuDeviceCreateBuffer(m_driver.device, &vertexBufferDesc);
	m_indexBuffer = wgpuDeviceCreateBuffer(m_driver.device, &indexBufferDesc);
	m_uniformBuffer = wgpuDeviceCreateBuffer(m_driver.device, &uniformBufferDesc);

	wgpuQueueWriteBuffer(m_driver.queue, m_vertexBuffer, 0, m_vertexData.data(), vertexBufferDesc.size);
	wgpuQueueWriteBuffer(m_driver.queue, m_indexBuffer, 0, m_indexData.data(), indexBufferDesc.size);

	float currentTime = 1.0f;
	wgpuQueueWriteBuffer(m_driver.queue, m_uniformBuffer, 0, &currentTime, sizeof(float));

	LOG_INFO("Buffers initialized");
}

Application::Application() {
	ZoneScoped;
	LOG_INFO("Application created");
}

bool Application::isRunning() {
	return !glfwWindowShouldClose(m_window.handle);
}

void Application::Terminate() {
	ZoneScoped;
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
	if (m_indexBuffer != nullptr) {
		wgpuBufferRelease(m_indexBuffer);
		LOG_TRACE("Index buffer destroyed");
	}
	if (m_uniformBuffer != nullptr) {
		wgpuBufferRelease(m_uniformBuffer);
		LOG_TRACE("Uniform buffer destroyed");
	}
	if (m_pipelineLayout != nullptr) {
		wgpuPipelineLayoutRelease(m_pipelineLayout);
		LOG_TRACE("Pipeline layout destroyed");
	}
	if (m_bindGroupLayout != nullptr) {
		wgpuBindGroupLayoutRelease(m_bindGroupLayout);
		LOG_TRACE("Bind group layout destroyed");
	}
	if (m_bindGroup != nullptr) {
		wgpuBindGroupRelease(m_bindGroup);
		LOG_TRACE("Bind group destroyed");
	}
	TerminateGui();
	glfwTerminate();
}

void Application::TerminateGui() {
	ZoneScoped;
	ImGui_ImplWGPU_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	LOG_INFO("GUI terminated");
}

Application::~Application() {
	ZoneScoped;
	LOG_INFO("Application destroyed");
}
