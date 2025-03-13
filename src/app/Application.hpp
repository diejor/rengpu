#pragma once

#include <glm.hpp>

#include "../renderer/Context.hpp"
#include "../renderer/Driver.hpp"
#include "../renderer/Vertex.hpp"

#include <GLFW/glfw3.h>

#include <vector>

class Application {
public:
	struct Window {
		GLFWwindow* handle;
		int width;
		int height;
	};

	bool initialize();
	bool initGui();
	void terminate();
	void terminateGui();
	void mainLoop();
	void updateGui();
	void onResize(const int& width, const int& height);
	bool isRunning();
	void initPipeline();
	void initBuffers();

	Window createWindow(int width, int height, const char* title);

	Application();
	~Application();

private:
	Window m_window;
	RdContext m_context;
	RdDriver m_driver;
    wgpu::raii::RenderPipeline m_pipeline;
    wgpu::raii::Buffer m_vertexBuffer;
    wgpu::raii::Buffer m_indexBuffer;
    wgpu::raii::Buffer m_uniformBuffer;
    wgpu::raii::PipelineLayout m_pipelineLayout;
    wgpu::raii::BindGroupLayout m_bindGroupLayout;
    wgpu::raii::BindGroup m_bindGroup;
	std::vector<Vertex> m_vertexData;
	std::vector<uint16_t> m_indexData;
	uint32_t m_indexCount;
};
