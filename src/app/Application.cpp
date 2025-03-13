#include "Application.hpp"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_wgpu.h"
#include "glfw3webgpu.h"
#include "logging_macros.h"
#include "tracy/Tracy.hpp"

#include <vector>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

// Callback for GLFW framebuffer resize
void onWindowResize(GLFWwindow* window, int width, int height) {
	auto that = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (that != nullptr) that->onResize(width, height);
}

Application::Window Application::createWindow(int width, int height, const char* title) {
	ZoneScoped;
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Window window = {
		.handle = glfwCreateWindow(width, height, title, nullptr, nullptr),
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

bool Application::initialize() {
	ZoneScoped;
	int width = 800;
	int height = 600;
	m_window = createWindow(width, height, "WebGPU");

	wgpu::Instance instance = wgpuCreateInstance(nullptr);
	LOG_TRACE("WebGPU instance created");

	wgpu::raii::Surface handle(glfwCreateWindowWGPUSurface(instance, m_window.handle));
	RdContext::Surface surface = {
		.handle = handle,
		.format = wgpu::TextureFormat::Undefined,  // configuration is deferred
		.depthTextureFormat = wgpu::TextureFormat::Undefined,
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
	};

	wgpu::Device device = nullptr;
	wgpu::Queue queue = nullptr;
	m_context.initialize(instance, std::move(surface), &device, &queue);
    wgpu::raii::Device h_device(device);
    wgpu::raii::Queue h_queue(queue);

	m_driver = { 
        .device = wgpu::raii::Device(device), 
        .queue = wgpu::raii::Queue(queue) };

	m_context.configureSurface(width, height, *m_driver.device);

	initBuffers();
	initPipeline();

	if (!initGui()) return false;

	LOG_INFO("Application initialized");
	return true;
}

bool Application::initGui() {
	ZoneScoped;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO();

	ImGui_ImplGlfw_InitForOther(m_window.handle, true);
#ifdef __EMSCRIPTEN__
	ImGui_ImplGlfw_InstallEmscriptenCallbacks(m_window.handle, "#canvas");
#endif

	// Use designated initializer to create a RenderInitInfo struct
	ImGui_ImplWGPU_InitInfo initInfo = {};
	initInfo.Device = m_driver.device;
	initInfo.NumFramesInFlight = 3;
	initInfo.RenderTargetFormat = m_context.surface.format;
	initInfo.DepthStencilFormat = m_context.surface.depthTextureFormat;

	ImGui_ImplWGPU_Init(&initInfo);

	LOG_INFO("GUI initialized");
	return true;
}

void Application::mainLoop() {
	FrameMark;
	ZoneScoped;
	glfwPollEvents();
	updateGui();

	{
		ZoneScopedN("Update Buffers");
		m_driver.queue.writeBuffer(*m_vertexBuffer, 0, m_vertexData.data(), m_vertexData.size() * sizeof(Vertex));
		float currentTime = static_cast<float>(glfwGetTime());
		m_driver.queue.writeBuffer(*m_uniformBuffer, 0, &currentTime, sizeof(float));
	}

	wgpu::TextureView textureView = m_context.nextTextureView();
	if (!textureView) return;

	wgpu::TextureView depthStencilView = m_driver.nextDepthView(m_context.surface);
	if (!depthStencilView) return;

	wgpu::CommandEncoder encoder = m_driver.device.createCommandEncoder(WGPUCommandEncoderDescriptor{
			.nextInChain = nullptr,
			.label = "My Command Encoder",
	});

	WGPURenderPassColorAttachment colorAttachment = {
		.nextInChain = nullptr,
		.view = textureView,
		.depthSlice = 0,
		.resolveTarget = nullptr,
		.loadOp = wgpu::LoadOp::Clear,
		.storeOp = wgpu::StoreOp::Store,
		.clearValue = { 0.1f, 0.1f, 0.1f, 1.0f },
	};

#ifndef EBGPU_BACKEND_WGPU
	colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif

	WGPURenderPassDepthStencilAttachment depthStencilAttachment = {
		.view = depthStencilView,
		.depthLoadOp = wgpu::LoadOp::Clear,
		.depthStoreOp = wgpu::StoreOp::Store,
		.depthClearValue = 1.0f,
		.depthReadOnly = false,
		.stencilLoadOp = wgpu::LoadOp::Clear,
		.stencilStoreOp = wgpu::StoreOp::Store,
		.stencilClearValue = 0,
		.stencilReadOnly = true,
	};

#ifndef WEBGPU_BACKEND_WGPU
	depthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Undefined;
	depthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Undefined;
#endif

	{
		ZoneScopedN("Render Pass");
		wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(WGPURenderPassDescriptor{
				.nextInChain = nullptr,
				.label = "My Render Pass",
				.colorAttachmentCount = 1,
				.colorAttachments = &colorAttachment,
				.depthStencilAttachment = &depthStencilAttachment,
				.occlusionQuerySet = nullptr,
				.timestampWrites = nullptr,
		});

		renderPass.setPipeline(*m_pipeline);
		renderPass.setVertexBuffer(0, *m_vertexBuffer, 0, m_vertexBuffer->getSize());
		renderPass.setIndexBuffer(*m_indexBuffer, wgpu::IndexFormat::Uint16, 0, m_indexBuffer->getSize());
		renderPass.setBindGroup(0, *m_bindGroup, 0, nullptr);
		renderPass.drawIndexed(m_indexCount, 1, 0, 0, 0);

		// Check framebuffer size before rendering ImGui
		{
			ZoneScopedN("FrameBuffer size check");
			int currentWidth, currentHeight;
			glfwGetFramebufferSize(m_window.handle, &currentWidth, &currentHeight);
			if (currentWidth == m_window.width && currentHeight == m_window.height) {
				ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
			}
		}

		renderPass.end();
		renderPass.release();
	}

	wgpu::CommandBuffer commandBuffer = encoder.finish(WGPUCommandBufferDescriptor{
			.nextInChain = nullptr,
			.label = "My Command Buffer",
	});

	encoder.release();
	m_driver.queue.submit(commandBuffer);
	commandBuffer.release();

	textureView.release();
	depthStencilView.release();
#ifndef __EMSCRIPTEN__
	m_context.surface.handle->present();
#endif

	m_context.polltick(m_driver.device);
}

void Application::onResize(const int& width, const int& height) {
	ZoneScoped;
	LOG_TRACE("Window resized to %d x %d", width, height);
	m_context.configureSurface(width, height, m_driver.device);
	m_window.width = width;
	m_window.height = height;
}

void Application::initPipeline() {
	ZoneScoped;

	m_bindGroupLayout = m_driver.createBindGroupLayout();
	m_bindGroup = m_driver.createBindGroup(*m_bindGroupLayout, *m_uniformBuffer);

	m_pipelineLayout = m_driver.device.createPipelineLayout(WGPUPipelineLayoutDescriptor{
			.nextInChain = nullptr,
			.label = "My Pipeline Layout",
			.bindGroupLayoutCount = 1,
			.bindGroupLayouts = (WGPUBindGroupLayout*)&m_bindGroupLayout,
	});

	m_pipeline = m_driver.createPipeline(m_context.surface, *m_pipelineLayout);

	LOG_INFO("Pipeline initialized");
}

void Application::updateGui() {
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
	ImGui::EndFrame();
	ImGui::Render();
}

uint32_t align(uint32_t offset, uint32_t align) {
	return (offset + align - 1) & ~(align - 1);
}

void Application::initBuffers() {
	ZoneScoped;
	bool loadGeometryQuery = m_driver.loadGeometry("pyramid.txt", m_vertexData, m_indexData);
	if (!loadGeometryQuery) {
		LOG_ERROR("Failed to load geometry");
	}

	m_indexCount = static_cast<uint32_t>(m_indexData.size());

	m_vertexBuffer = m_driver.device.createBuffer(WGPUBufferDescriptor{
			.nextInChain = nullptr,
			.label = "My Vertex Buffer",
			.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
			.size = align(m_vertexData.size() * sizeof(Vertex), 4),
			.mappedAtCreation = false,
	});

	m_indexBuffer = m_driver.device.createBuffer(WGPUBufferDescriptor{
			.nextInChain = nullptr,
			.label = "My Index Buffer",
			.usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst,
			.size = align(m_indexData.size() * sizeof(uint16_t), 4),
			.mappedAtCreation = false,
	});

	m_uniformBuffer = m_driver.device.createBuffer(WGPUBufferDescriptor{
			.nextInChain = nullptr,
			.label = "My Uniform Buffer",
			.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
			.size = align(sizeof(float), 16),
			.mappedAtCreation = false,
	});

	m_driver.queue.writeBuffer(*m_vertexBuffer, 0, m_vertexData.data(), m_vertexBuffer->getSize());
	m_driver.queue.writeBuffer(*m_indexBuffer, 0, m_indexData.data(), m_indexBuffer->getSize());

	float currentTime = 1.0f;
	m_driver.queue.writeBuffer(*m_uniformBuffer, 0, &currentTime, m_uniformBuffer->getSize());

	LOG_INFO("Buffers initialized");
}

bool Application::isRunning() {
	return !glfwWindowShouldClose(m_window.handle);
}

void Application::terminate() {
	ZoneScoped;
	if (m_window.handle != nullptr) {
		glfwDestroyWindow(m_window.handle);
		LOG_TRACE("Application window destroyed");
	}
	terminateGui();
	glfwTerminate();
}

void Application::terminateGui() {
	ZoneScoped;
	ImGui_ImplWGPU_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	LOG_INFO("GUI terminated");
}

Application::Application() {
	ZoneScoped;
	LOG_INFO("Application created");
}

Application::~Application() {
	ZoneScoped;
	LOG_INFO("Application destroyed");
}
