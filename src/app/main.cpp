#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
include<emscripten.h>
#endif

#include <cassert>
#include <iostream>


int main() {
    // We create a descriptor
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    WGPUInstance instance = wgpuCreateInstance(nullptr);
#else
    WGPUInstance instance = wgpuCreateInstance(&desc);
#endif

    if (!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }
    return 0;
}
