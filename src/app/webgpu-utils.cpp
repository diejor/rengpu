/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2024 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "webgpu-utils.h"
#include <webgpu/webgpu.h>
#include <cstring>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

#include <iostream>
#include <cassert>

WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options) {
	struct UserData {
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	WGPURequestAdapterCallback onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* pUserData, void*) {
				UserData& userData = *reinterpret_cast<UserData*>(pUserData);
                std::cout << "Adapter request ended" << std::endl;
				if (status == WGPURequestAdapterStatus_Success) {
                    std::cout << "Adapter created: " << adapter << std::endl;
					userData.adapter = adapter;
				} else {
					std::cout << "Could not get WebGPU adapter: " << message.data << std::endl;
				}
				userData.requestEnded = true;
			};

    WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
    adapterCallbackInfo.callback = onAdapterRequestEnded;
    adapterCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    adapterCallbackInfo.userdata1 = (void*)&userData;


	WGPUFuture adapterFuture = wgpuInstanceRequestAdapter(instance, options, adapterCallbackInfo);


    std::cout << "Waiting for adapter request to end" << std::endl;
    WGPUFutureWaitInfo waitInfo = {};
    waitInfo.future = adapterFuture;
    waitInfo.completed = userData.requestEnded;

#ifdef __EMSCRIPTEN__
	while (!userData.requestEnded) {
		emscripten_sleep(100);
	}
#endif	// __EMSCRIPTEN__

    std::cout << "Request ended" << std::endl;
	assert(userData.requestEnded);

	return userData.adapter;
}

void inspectAdapter(WGPUAdapter adapter) {
    //WGPUSupportedFeatures supportedFeatures = {};

	// Call the function a first time with a null return address, just to get
	// the entry count.
	//wgpuAdapterGetFeatures(adapter, &supportedFeatures);

	//std::cout << "Adapter features:" << std::endl;
    //std::cout << std::hex; // Print hex numbers
    //for (uint32_t i = 0; i < supportedFeatures.featureCount; ++i) {
    //    WGPUFeatureName feature = supportedFeatures.features[i];
    //    std::cout << " - " << feature << std::endl;
    //}
	//std::cout << std::dec; // Restore decimal numbers
    //
	WGPUAdapterInfo adapterInfo= {};
    wgpuAdapterGetInfo(adapter, &adapterInfo);
    std:: cout << "Adapter device: " << adapterInfo.device.data << std::endl;
    std:: cout << "Adapter deviceID: " << adapterInfo.deviceID << std::endl;
    std:: cout << "Adapter vendor: " << adapterInfo.vendor.data << std::endl;
    std:: cout << "Adapter vendorID: " << adapterInfo.vendorID << std::endl;
    std:: cout << "Adapter architecture: " << adapterInfo.architecture.data << std::endl;
    std:: cout << "Adapter description: " << adapterInfo.description.data << std::endl;

    std::cout << std::hex; // Print hex numbers
    std::cout << "Adapter type: 0x" << adapterInfo.adapterType << std::endl;
    std::cout << "Adapter backend: 0x" << adapterInfo.backendType << std::endl;
    std::cout << std::dec; // Restore decimal numbers
}



WGPUDevice requestDeviceSync(WGPUInstance instance, WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor) {
	struct UserData {
		WGPUDevice device = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	WGPURequestDeviceCallback onDeviceRequestEnded =
			[](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* pUserData, void*) {
				UserData& userData = *reinterpret_cast<UserData*>(pUserData);
				if (status == WGPURequestDeviceStatus_Success) {
					userData.device = device;
				} else {
					std::cout << "Could not get WebGPU device: " << message.data << std::endl;
				}
				userData.requestEnded = true;
			};

    WGPURequestDeviceCallbackInfo deviceCallbackInfo = {};
    deviceCallbackInfo.callback = onDeviceRequestEnded;
    deviceCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    deviceCallbackInfo.userdata1 = (void*)&userData;
    deviceCallbackInfo.userdata2 = nullptr;


	wgpuAdapterRequestDevice(adapter, descriptor, deviceCallbackInfo);

#ifdef __EMSCRIPTEN__
	while (!userData.requestEnded) {
		emscripten_sleep(100);
	}
#endif	// __EMSCRIPTEN__

	assert(userData.requestEnded);

	return userData.device;
}
