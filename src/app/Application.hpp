#pragma once

#include <glm.hpp>

#include "../renderer/Context.hpp"
#include "../renderer/Vertex.hpp"
#include "webgpu/webgpu.h"

#include <GLFW/glfw3.h>

#include <vector>

class Application {
public:
	struct Window {
		GLFWwindow* handle;
		int width;
		int height;
	};

	bool Initialize();
	bool InitGui();
	void Terminate();
	void TerminateGui();
	void MainLoop();
	void UpdateGui();
	void onResize(const int& width, const int& height);
	bool isRunning();
	void InitPipeline();
	void InitBuffers();

	Window CreateWindow(int width, int height, const char* title);

	Application();
	~Application();

private:
	Window m_window;
	RdContext m_context;
	RdDriver m_driver;
	WGPURenderPipeline m_pipeline;
	WGPUBuffer m_vertexBuffer;
	WGPUBuffer m_indexBuffer;
	WGPUBuffer m_uniformBuffer;
	WGPUPipelineLayout m_pipelineLayout;
	WGPUBindGroupLayout m_bindGroupLayout;
	WGPUBindGroup m_bindGroup;
	std::vector<Vertex> m_vertexData;
	std::vector<uint16_t> m_indexData;
	uint32_t m_indexCount;
};
