#pragma once

#include "webgpu/webgpu.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

class Application {
public:
	bool initialize();
	void terminate();
    void main_loop();
    bool is_running();
private:
    WGPUTextureView next_texture_view();

    SDL_Window* _window;
    WGPUDevice _device;
    WGPUQueue _queue;
    WGPUSurface _surface;
};
