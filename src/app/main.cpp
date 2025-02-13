#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
include<emscripten.h>
#endif

#include <cassert>
#include <iostream>

    using namespace std;

WGPUAdapter requestAdapterSync(WGPUInstance instance,
                               WGPURequestAdapterOptions const* options) {
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status,
                                    WGPUAdapter adapter, char const* message,
                                    void* pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        } else {
            std::cout << "Could not get WebGPU adapter: " << message
                      << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuInstanceRequestAdapter(instance, options, onAdapterRequestEnded,
                               (void*)&userData);

#ifdef __EMSCRIPTEN__
    while (!userData.requestEnded) {
        emscripten_sleep(100);
    }
#endif  // __EMSCRIPTEN__

    assert(userData.requestEnded);

    return userData.adapter;
}

int main() {
    // We create a descriptor
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    WGPUInstance instance = wgpuCreateInstance(nullptr);
#else
    WGPUInstance instance = wgpuCreateInstance(&desc);
#endif

    cout << "Instance created: " << instance << endl;

    WGPURequestAdapterOptions adapterOptions = {};
    adapterOptions.nextInChain = nullptr;
    WGPUAdapter adapter = requestAdapterSync(instance, &adapterOptions);

    cout << "Adapter created: " << adapter << endl;
    cout << "Release instace" << endl;
    wgpuInstanceRelease(instance);

    WGPUAdapterInfo adapterInfo {};
    WGPUStatus status = wgpuAdapterGetInfo(adapter, &adapterInfo);

    cout << "Adapter info status: " << status << endl;

    cout << "Adapter properties:" << endl;
    cout << " ~ device: " << adapterInfo.deviceID << endl;
    cout << " ~ vendor: " << adapterInfo.vendor << endl;
    
    return 0;
}
