#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
include<emscripten.h>
#endif

#include <cassert>
#include <iostream>

		using namespace std;

WGPUAdapter requestAdapterSync(WGPUInstance instance, const WGPURequestAdapterOptions* options) {
	struct UserData {
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	auto onAdapterRequestEnded =
			[](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* pUserData) {
				UserData& userData = *reinterpret_cast<UserData*>(pUserData);
				if (status == WGPURequestAdapterStatus_Success) {
					userData.adapter = adapter;
				} else {
					std::cout << "Could not get WebGPU adapter: " << message << std::endl;
				}
				userData.requestEnded = true;
			};

	wgpuInstanceRequestAdapter(instance, options, onAdapterRequestEnded, (void*)&userData);

#ifdef __EMSCRIPTEN__
	while (!userData.requestEnded) {
		emscripten_sleep(100);
	}
#endif	// __EMSCRIPTEN__

	assert(userData.requestEnded);

	return userData.adapter;
}

WGPUDevice requestDeviceSync(WGPUAdapter adapter, const WGPUDeviceDescriptor* descriptor) {
	struct UserData {
		WGPUDevice device = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* pUserData) {
		UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        } else {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
		userData.requestEnded = true;
	};

	wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded, (void*)&userData);

#ifdef __EMSCRIPTEN__
	while (!userData.requestEnded) {
		emscripten_sleep(100);
	}
#endif	// __EMSCRIPTEN__

	assert(userData.requestEnded);

	return userData.device;
}

int main() {


    cout << "Requesting instance" << endl;
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_EMSCRIPTEN
	WGPUInstance instance = wgpuCreateInstance(nullptr);
#else
	WGPUInstance instance = wgpuCreateInstance(&desc);
#endif
    cout << "Instance created: " << instance << endl;

    cout << "Requesting adapter" << endl;
	WGPURequestAdapterOptions adapterOptions = {}; // if not {} then it will be garbage and requestAdapterSync will not find the adapter
	adapterOptions.nextInChain = nullptr;
	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOptions);
    cout << "Adapter created: " << adapter << endl;
	wgpuInstanceRelease(instance);

    cout << "Requesting device" << endl;
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "Device";
    deviceDesc.requiredFeatureCount= 0;
    deviceDesc.requiredLimits = nullptr;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "Default Queue";
    deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, const char* message, void*) {
        cout << "Device lost: reason " << reason << endl;
        if (message != nullptr) {
            cout << "Message: " << message << endl;
        }
        cout << endl;
    };

    WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);
    cout << "Device created: " << device << endl;
    wgpuAdapterRelease(adapter);

    auto onDeviceError = [](WGPUErrorType type, const char* message, void*) {
        cout << "Uncaptured device error: type " << type;
        if (message != nullptr) {
            cout << ", message: " << message;
        }
        cout << endl;
    };

    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr);

    

    wgpuDeviceRelease(device);
	return 0;
}
