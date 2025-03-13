#include <webgpu/webgpu.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define WEBGPU_CPP_IMPLEMENTATION
#include "Driver.hpp"
#include "logging_macros.h"

#include <tracy/Tracy.hpp>
#include <webgpu/webgpu.hpp>

wgpu::RenderPipeline RdDriver::createPipeline(RdContext::Surface& rdSurface, const wgpu::PipelineLayout& pipelineLayout) {
	ZoneScoped;
	// Load shader module using wrapper method.
	wgpu::ShaderModule module = loadShaderModule("triangles.wgsl");

	// For vertex attributes and other descriptor types that are not handles, we use the WGPU C types.
	WGPUVertexAttribute positionAtt = {
		.format = wgpu::VertexFormat::Float32x3,
		.offset = 0,
		.shaderLocation = 0,
	};

	WGPUVertexAttribute colorAtt = {
		.format = wgpu::VertexFormat::Float32x3,
		.offset = 3 * sizeof(float),
		.shaderLocation = 1,
	};

	std::vector<WGPUVertexAttribute> attributes = { positionAtt, colorAtt };

	WGPUVertexBufferLayout vertexBufferLayout = {
		.arrayStride = 6 * sizeof(float),
		.stepMode = wgpu::VertexStepMode::Vertex,
		.attributeCount = 2,
		.attributes = attributes.data(),
	};

	WGPUBlendState blendState = {
        .color = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::SrcAlpha,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
        },
        .alpha = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::Zero,
            .dstFactor = wgpu::BlendFactor::One,
        },
    };

	WGPUColorTargetState colorTargetState = {
		.nextInChain = nullptr,
		.format = rdSurface.format,
		.blend = &blendState,
		.writeMask = wgpu::ColorWriteMask::All,
	};

	WGPUFragmentState fragmentState = {
		.nextInChain = nullptr,
		.module = module,  // module is a handle, passed as required
		.entryPoint = "fs_main",
		.constantCount = 0,
		.constants = nullptr,
		.targetCount = 1,
		.targets = &colorTargetState,
	};

	WGPUDepthStencilState depthStencilState = {
        .nextInChain = nullptr,
        .format = rdSurface.depthTextureFormat,
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less,
        .stencilFront = {
            .compare = wgpu::CompareFunction::Always,
            .failOp = wgpu::StencilOperation::Keep,
            .depthFailOp = wgpu::StencilOperation::Keep,
            .passOp = wgpu::StencilOperation::Keep,
        },
        .stencilBack = {
            .compare = wgpu::CompareFunction::Always,
            .failOp = wgpu::StencilOperation::Keep,
            .depthFailOp = wgpu::StencilOperation::Keep,
            .passOp = wgpu::StencilOperation::Keep,
        },
        .stencilReadMask = 0x00,
        .stencilWriteMask = 0x00,
        .depthBias = 0,
        .depthBiasSlopeScale = 0.0f,
        .depthBiasClamp = 0.0f,
    };

	wgpu::RenderPipeline pipeline = device.createRenderPipeline(WGPURenderPipelineDescriptor {
        .nextInChain = nullptr,
        .label = "My Pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .nextInChain = nullptr,
            .module = module,
            .entryPoint = "vs_main",
            .constantCount = 0,
            .constants = nullptr,
            .bufferCount = 1,
            .buffers = &vertexBufferLayout,
        },
        .primitive = {
            .nextInChain = nullptr,
            .topology = wgpu::PrimitiveTopology::TriangleList,
            .stripIndexFormat = wgpu::IndexFormat::Undefined,
            .frontFace = wgpu::FrontFace::CCW,
            .cullMode = wgpu::CullMode::None,
        },
        .depthStencil = &depthStencilState,
        .multisample = {
            .nextInChain = nullptr,
            .count = 1,
            .mask = ~0u,
            .alphaToCoverageEnabled = false,
        },
        .fragment = &fragmentState,
    });

	LOG_INFO("Pipeline created");
	return pipeline;
}

wgpu::BindGroup RdDriver::createBindGroup(const wgpu::BindGroupLayout& layout, wgpu::Buffer& buffer) {
	ZoneScoped;
	WGPUBindGroupEntry bindGroupEntry = {
		.nextInChain = nullptr,
		.binding = 0,
		.buffer = buffer,
		.offset = 0,
		.size = buffer.getSize(),
		.sampler = nullptr,
		.textureView = nullptr,
	};

    wgpu::BindGroup bindGroup = device.createBindGroup(WGPUBindGroupDescriptor{
        .nextInChain = nullptr,
        .label = "My Bind Group",
        .layout = layout,
        .entryCount = 1,
        .entries = &bindGroupEntry,
    });

    return bindGroup;
}

wgpu::BindGroupLayout RdDriver::createBindGroupLayout() {
	ZoneScoped;
	WGPUBindGroupLayoutEntry bindGroupLayoutEntry = {
        .nextInChain = nullptr,
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex,
        .buffer = {
            .nextInChain = nullptr,
            .type = wgpu::BufferBindingType::Uniform,
            .hasDynamicOffset = false,
            .minBindingSize = 4 * sizeof(float),
        },
        .sampler = {
            .nextInChain = nullptr,
            .type = wgpu::SamplerBindingType::Undefined,
        },
        .texture = {
            .nextInChain = nullptr,
            .sampleType = wgpu::TextureSampleType::Undefined,
            .viewDimension = wgpu::TextureViewDimension::Undefined,
            .multisampled = false,
        },
        .storageTexture = {
            .nextInChain = nullptr,
            .access = wgpu::StorageTextureAccess::Undefined,
            .format = wgpu::TextureFormat::Undefined,
            .viewDimension = wgpu::TextureViewDimension::Undefined,
        },
    };

    wgpu::BindGroupLayout bindGroupLayout = device.createBindGroupLayout(WGPUBindGroupLayoutDescriptor{
        .nextInChain = nullptr,
        .label = "My Bind Group Layout",
        .entryCount = 1,
        .entries = &bindGroupLayoutEntry,
    });
    
    return bindGroupLayout;
}

wgpu::ShaderModule RdDriver::loadShaderModule(const std::filesystem::path& filename) {
	ZoneScoped;
	std::ifstream file(std::string(RESOURCE_DIR) / filename, std::ios::binary);
	if (!file) {
		throw std::runtime_error("Failed to open shader: " + filename.string());
	}
	LOG_TRACE("Shader file opened: %s", filename.c_str());
	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	std::string shaderSource(size, ' ');
	file.seekg(0);
	file.read(shaderSource.data(), size);

	/*	// for some reason this doesnt work*/
	/*	WGPUShaderModuleWGSLDescriptor shaderDesc = {*/
	/*	        .chain = {*/
	/*		        .next = nullptr,*/
	/*		        .sType = wgpu::SType::ShaderModuleWGSLDescriptor,*/
	/*		    },*/
	/*		    .code = shaderSource.c_str(),*/
	/*		};*/
	/*	return device.createShaderModule(WGPUShaderModuleDescriptor{*/
	/*			.nextInChain = &shaderDesc.chain,*/
	/*			.label = filename.c_str(),*/
	/*#ifdef WEBGPU_BACKEND_WGPU*/
	/*			.hintCount = 0,*/
	/*			.hints = nullptr,*/
	/*#endif*/
	/*	});*/

	wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc{};
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
	shaderCodeDesc.code = shaderSource.c_str();

	wgpu::ShaderModuleDescriptor shaderDesc{};
#ifdef WEBGPU_BACKEND_WGPU
	shaderDesc.hintCount = 0;
	shaderDesc.hints = nullptr;
#endif
	shaderDesc.nextInChain = &shaderCodeDesc.chain;
	return device.createShaderModule(shaderDesc);
}

bool RdDriver::loadGeometry(
		const std::filesystem::path& path,
		std::vector<Vertex>& vertices,
		std::vector<uint16_t>& indices
) {
	ZoneScoped;
	std::ifstream file(std::string(RESOURCE_DIR) / path);
	if (!file.is_open()) {
		return false;
	}

	vertices.clear();
	indices.clear();

	enum class Section { None, Points, Indices };
	Section currentSection = Section::None;

	float value;
	uint16_t index;
	std::string line;
	while (std::getline(file, line)) {
		// Remove CR if present
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}

		if (line == "[points]") {
			currentSection = Section::Points;
		} else if (line == "[indices]") {
			currentSection = Section::Indices;
		} else if (line.empty() || line[0] == '#') {
			continue;
		} else if (currentSection == Section::Points) {
			std::istringstream iss(line);
			Vertex vertex;
			for (int i = 0; i < 6; ++i) {
				iss >> value;
				if (i == 0)
					vertex.position.x = value;
				else if (i == 1)
					vertex.position.y = value;
				else if (i == 2)
					vertex.position.z = value;
				else if (i == 3)
					vertex.color.r = value;
				else if (i == 4)
					vertex.color.g = value;
				else if (i == 5)
					vertex.color.b = value;
			}
			vertices.push_back(vertex);
		} else if (currentSection == Section::Indices) {
			std::istringstream iss(line);
			for (int i = 0; i < 3; ++i) {
				iss >> index;
				indices.push_back(index);
			}
		}
	}
	LOG_INFO("Geometry loaded: %s", path.c_str());
	return true;
}

wgpu::TextureView RdDriver::nextDepthView(RdSurface& rdSurface) {
	ZoneScoped;

	wgpu::Texture depthTexture = device.createTexture(WGPUTextureDescriptor{
			.nextInChain = nullptr,
			.label = "Depth Texture",
			.usage = wgpu::TextureUsage::RenderAttachment,
			.dimension = wgpu::TextureDimension::_2D,
			.size = { rdSurface.width, rdSurface.height, 1 },
			.format = rdSurface.depthTextureFormat,
			.mipLevelCount = 1,
			.sampleCount = 1,
			.viewFormatCount = 1,
			.viewFormats = (WGPUTextureFormat*)&rdSurface.depthTextureFormat,
	});

	wgpu::TextureView depthView = depthTexture.createView(WGPUTextureViewDescriptor{
			.nextInChain = nullptr,
			.label = "Depth texture view",
			.format = rdSurface.depthTextureFormat,
			.dimension = wgpu::TextureViewDimension::_2D,
			.baseMipLevel = 0,
			.mipLevelCount = 1,
			.baseArrayLayer = 0,
			.arrayLayerCount = 1,
			.aspect = wgpu::TextureAspect::DepthOnly,
	});

	depthTexture.release();

	return depthView;
}
