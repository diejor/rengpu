#include <webgpu-utils.h>
#include <webgpu/webgpu.h>

#ifdef WEBGPU_BACKEND_WGPU
#  include <webgpu/wgpu.h>
#endif // WEBGPU_BACKEND_WGPU

#include <cassert>
#include <cstring>
#include <iostream>

int main() {
	WGPUInstanceDescriptor instanceDesc = {};
	instanceDesc.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
	// Make sure the uncaptured error callback is called as soon as an error
	// occurs rather than at the next call to "wgpuDeviceTick".
	WGPUDawnTogglesDescriptor toggles = {};
	toggles.chain.next = nullptr;
	toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
	toggles.disabledToggleCount = 0;
	toggles.enabledToggleCount = 1;
	const char* toggleName = "enable_immediate_error_handling";
	toggles.enabledToggles = &toggleName;

	instanceDesc.nextInChain = &toggles.chain;
#endif

#ifdef __EMSCRIPTEN__
	WGPUInstance instance = wgpuCreateInstance(nullptr);
#else
	WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
#endif	// __EMSCRIPTEN__
	std::cout << "Instance created: " << instance << std::endl;

	WGPURequestAdapterOptions adapterOptions = {};
	adapterOptions.nextInChain = nullptr;

	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOptions);
	std::cout << "Adapter created: " << adapter << std::endl;
	wgpuInstanceRelease(instance);

	WGPUDeviceDescriptor deviceDesc = {};
	deviceDesc.label = "My Device";
	deviceDesc.nextInChain = nullptr;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.requiredFeatureCount = 0;
	deviceDesc.requiredLimits = nullptr;

	deviceDesc.deviceLostCallbackInfo.nextInChain = nullptr;
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
	deviceDesc.deviceLostCallbackInfo.callback =
			[](const WGPUDevice* device, WGPUDeviceLostReason reason, const char* message, void*) {
				std::cout << "Device lost: " << device << ", reason: " << reason;
				if (message != nullptr) {
					std::cout << ", message: " << message;
				};
				std::cout << std::endl;
			};
	deviceDesc.deviceLostCallbackInfo.userdata = nullptr;

	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "My Queue";

	WGPUUncapturedErrorCallbackInfo uncapturedErrorCallbackInfo = {};
	uncapturedErrorCallbackInfo.nextInChain = nullptr;
	uncapturedErrorCallbackInfo.userdata = nullptr;
	uncapturedErrorCallbackInfo.callback = [](WGPUErrorType type, const char* message, void*) {
		std::cout << "Uncaptured error: type " << type;
		if (message != nullptr) {
			std::cout << ", message: " << message;
		};
		std::cout << std::endl;
	};

	WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);
	std::cout << "Device created: " << device << std::endl;
	wgpuAdapterRelease(adapter);

	WGPUQueue queue = wgpuDeviceGetQueue(device);
	WGPUQueueWorkDoneCallback onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* userdata) {
		std::cout << "Work done: " << status << std::endl;
	};
	wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr);

	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = "My Encoder";
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

	WGPUCommandBufferDescriptor commandBufferDesc = {};
	commandBufferDesc.nextInChain = nullptr;
	commandBufferDesc.label = "My Command Buffer";
	WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
	wgpuCommandEncoderRelease(encoder);

	std::cout << "Submitting command buffer" << std::endl;
	wgpuQueueSubmit(queue, 1, &commandBuffer);
	wgpuCommandBufferRelease(commandBuffer);
	std::cout << "Command buffer submitted" << std::endl;

    // to avoid this use context boolean in onQueueWorkDone and wait for it to be true
    // im thinking that with WGPUFuture this might get simplified/unified
    // requesting an adapter and a device might work the same way
	for (int i = 0; i < 5; ++i) {
		std::cout << "Tick/Poll device..." << std::endl;
#if defined(WEBGPU_BACKEND_DAWN)
		wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
		wgpuDevicePoll(device, false, nullptr);
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
		emscripten_sleep(100);
#endif
	}

	// Maybe unique_ptr?
	wgpuQueueRelease(queue);
	wgpuDeviceRelease(device);
}
