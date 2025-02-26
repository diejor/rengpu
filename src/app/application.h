#pragma once

#include "renderer_context.h"
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
	GLFWwindow* getWindowHandle() const {
		return m_window.handle;
	}

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
    bool m_skipFrame;
    bool m_insideView;
	WGPURenderPipeline m_pipeline;
	WGPUBuffer m_vertexBuffer;
	std::vector<Vertex> m_vertexData;
	uint32_t m_vertexCount;
};
