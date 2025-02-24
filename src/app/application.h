#pragma once

#include "webgpu/webgpu.h"

#include <GLFW/glfw3.h>

#include "renderer_context.h"

class Application {
public:
    struct Window {
        GLFWwindow* handle;
        uint32_t width;
        uint32_t height;
    };

	bool initialize();
	void terminate();
	void mainLoop();
	bool isRunning();
    void initPipeline();
    
    Window createWindow(uint32_t width, uint32_t height, const char* title);
    GLFWwindow* getWindowHandle() const {
        return m_window.handle;
    }

    Application();
    ~Application();
private:
	Window m_window;
	RDContext m_context;
    WGPURenderPipeline m_pipeline;
};
