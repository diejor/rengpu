#pragma once

#include "Surface.hpp"
#include "Vertex.hpp"
#include <filesystem>
#include <vector>

#include <webgpu/webgpu.hpp>

struct RdDriver {
    wgpu::RenderPipeline createPipeline(const RdSurface& p_rdSurface, const wgpu::PipelineLayout& p_pipelineLayout);
    wgpu::BindGroupLayout createBindGroupLayout();
    wgpu::BindGroup createBindGroup(const wgpu::BindGroupLayout& p_layout, wgpu::Buffer& p_buffer);
    wgpu::TextureView nextDepthView(const RdSurface& rdSurface);
    wgpu::ShaderModule loadShaderModule(const std::filesystem::path& filename);
	bool loadGeometry(
			const std::filesystem::path& filename,
			std::vector<Vertex>& vertices,
			std::vector<uint16_t>& indices
	);

    wgpu::Device device;
    wgpu::Queue queue;
};

