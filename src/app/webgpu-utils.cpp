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
				if (status == WGPURequestAdapterStatus_Success) {
					userData.adapter = adapter;
				} else {
					std::cout << "Could not get WebGPU adapter: " << message.data << std::endl;
				}
				userData.requestEnded = true;
			};

    WGPURequestAdapterCallbackInfo adapterCallbackInfo = {};
    adapterCallbackInfo.callback = onAdapterRequestEnded;
    adapterCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    adapterCallbackInfo.userdata1 = (void*)&userData;
    adapterCallbackInfo.userdata2 = nullptr;

	wgpuInstanceRequestAdapter(instance, options, adapterCallbackInfo);

#ifdef __EMSCRIPTEN__
	while (!userData.requestEnded) {
		emscripten_sleep(100);
	}
#endif	// __EMSCRIPTEN__

	assert(userData.requestEnded);

	return userData.adapter;
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

WGPUStringView wgpuStringViewFromCString(const char* str) {
    WGPUStringView view;
    view.data = str;
    view.length = strlen(str);
    return view;
}
