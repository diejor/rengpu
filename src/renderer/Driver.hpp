#pragma once

#include "Surface.hpp"
#include "Vertex.hpp"
#include <webgpu/webgpu.h>
#include <filesystem>
#include <vector>

struct RdDriver {
    WGPURenderPipeline PipelineCreate(const RdSurface& p_rdSurface, const WGPUPipelineLayout& p_pipelineLayout);
    WGPUBindGroupLayout BindGroupLayoutCreate();
    WGPUBindGroup BindGroupCreate(const WGPUBindGroupLayout& p_layout, const WGPUBuffer& p_buffer);
    WGPUTextureView NextDepthView(const RdSurface& rdSurface);
    WGPUShaderModule ShaderModuleLoad(const std::filesystem::path& filename);
	bool GeometryLoad(
			const std::filesystem::path& filename,
			std::vector<Vertex>& vertices,
			std::vector<uint16_t>& indices
	);

	WGPUDevice device;
	WGPUQueue queue;
};

