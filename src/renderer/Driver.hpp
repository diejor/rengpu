#pragma once

#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "Vertex.hpp"
#include "Context.hpp"
#include <filesystem>
#include <vector>


struct RdDriver {
    wgpu::RenderPipeline createPipeline(RdContext::Surface& p_rdSurface, const wgpu::PipelineLayout& p_pipelineLayout);
    wgpu::BindGroupLayout createBindGroupLayout();
    wgpu::BindGroup createBindGroup(const wgpu::BindGroupLayout& p_layout, wgpu::Buffer& p_buffer);
    wgpu::TextureView nextDepthView(RdContext::Surface& rdSurface);
    wgpu::ShaderModule loadShaderModule(const std::filesystem::path& filename);
	bool loadGeometry(
			const std::filesystem::path& filename,
			std::vector<Vertex>& vertices,
			std::vector<uint16_t>& indices
	);

    wgpu::raii::Device device;
    wgpu::raii::Queue queue;
};

