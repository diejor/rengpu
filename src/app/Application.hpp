#pragma once

#include "../renderer/RendererContext.hpp"
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

	struct Position {
		float x, y;
	};

	struct Color {
		float r, g, b;
	};

	struct Vertex {
		Position position;
		Color color;
	};

private:
	Window m_window;
	RDContext m_context;
	WGPURenderPipeline m_pipeline;
	WGPUBuffer m_vertexBuffer;
	std::vector<Vertex> m_vertexData;
	uint32_t m_vertexCount;
};
