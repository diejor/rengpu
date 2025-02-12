#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
include<emscripten.h>
#endif

#include <cassert>
#include <iostream>

    WGPUAdapter requestAdapterSync(WGPUInstance instance,
                                   WGPURequestAdapterOptions const* options) {
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    WGPURequestAdapterCallback ondapterRequestEnded =
        [](WGPURequestAdapterStatus status, WGPUAdapter adapter,
           WGPUStringView message, void* userdata1, [[maybe_unused]] void* userdata2) {
            UserData& userData = *reinterpret_cast<UserData*>(userdata1);
            if (status == WGPURequestAdapterStatus_Success) {
                std::cout << "Adapter request succeeded!" << std::endl;
                userData.adapter = adapter;
            } else {
                std::cout << "Could not get WebGPU adapter: "
                          << (char*)message.data << std::endl;
            }
            userData.requestEnded = true;
        };

    WGPURequestAdapterCallbackInfo callbackInfo = {};
    callbackInfo.nextInChain = nullptr;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = ondapterRequestEnded;
    callbackInfo.userdata1 = (void*)&userData;

    std::cout << "Requesting adapter..." << std::endl;
    // wgpuInstanceRequestAdapter(instance, options, callbackInfo);
    WGPUFuture future = wgpuInstanceRequestAdapter(instance, options, callbackInfo);

    WGPUFutureWaitInfo waitInfo = {};
    waitInfo.future = future;
    waitInfo.completed = false;
    
    wgpuInstanceWaitAny(instance, 1, &waitInfo, 0);


    return userData.adapter;
}

int main() {
    // We create a descriptor
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

    WGPUInstance instance = wgpuCreateInstance(&desc);

    if (!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);

    std::cout << "Got adapter: " << adapter << std::endl;

    return 0;
}
