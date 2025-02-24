#pragma once

#include <webgpu/webgpu.h>

class Window;

class RendererContext {

public:
    // The application is responsible for managing the surface, the context only configures it
    struct Surface {
        WGPUSurface surface;
        WGPUTextureFormat format;
        uint32_t width;
        uint32_t height;
    };

    void initialize(WGPUInstance instance, Surface surface);
    void configureSurface(Surface& p_surface);

    RendererContext();
	~RendererContext();

    RendererContext& operator=(const RendererContext&) = delete;
    RendererContext& operator=(const RendererContext&&) = delete;

    WGPUInstance getInstance() const {
        return m_instance;
    }
    WGPUSurface getSurface() const {
        return m_surface.surface;
    }
	WGPUAdapter getAdapter() const {
		return m_adapter;
	}
	WGPUDevice getDevice() const {
		return m_device;
	}
	WGPUQueue getQueue() const {
		return m_queue;
	}

private:
    WGPUInstance m_instance;
    Surface m_surface;
	WGPUAdapter m_adapter;
	WGPUDevice m_device;
	WGPUQueue m_queue;
    bool m_waitingAsync;
};
