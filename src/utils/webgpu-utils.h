#include <webgpu/webgpu.h>
#include <cassert>
#include <iostream>
#include <ostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif	// __EMSCRIPTEN__

inline WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options) {
	struct UserData {
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	WGPURequestAdapterCallback onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* pUserData) {
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

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUDevice device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
inline WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor) {
	struct UserData {
		WGPUDevice device = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	WGPURequestDeviceCallback onDeviceRequestEnded =
			[](WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* pUserData) {
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
