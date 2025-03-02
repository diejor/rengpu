#include "Surface.hpp"
#include <webgpu/webgpu.h>
#include <string>

struct RdDriver {
    WGPURenderPipeline PipelineCreate(const RdSurface& p_rdSurface, const WGPUPipelineLayout& p_pipelineLayout);
    WGPUBindGroupLayout BindGroupLayoutCreate();
    WGPUBindGroup BindGroupCreate(const WGPUBindGroupLayout& p_layout, const WGPUBuffer& p_buffer);
    WGPUShaderModule ShaderModuleLoad(const std::string& filename);

	WGPUDevice device;
	WGPUQueue queue;
};

