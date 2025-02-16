#pragma once

#include "webgpu/webgpu.h"
#include <GLFW/glfw3.h>

class Application {
public:
	bool initialize();
	void terminate();
    void mainLoop();
    bool isRunning();
private:
    WGPUTextureView nextTextureView();

    GLFWwindow* _window;
    WGPUDevice _device;
    WGPUQueue _queue;
    WGPUSurface _surface;
};
