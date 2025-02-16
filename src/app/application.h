#pragma once

#include "webgpu/webgpu.h"
#include <GLFW/glfw3.h>

class Application {
public:
	bool initialize();
	void terminate();
    void main_loop();
    bool is_running();
private:
    WGPUTextureView next_texture_view();

    GLFWwindow* _window;
    WGPUDevice _device;
    WGPUQueue _queue;
    WGPUSurface _surface;
};
