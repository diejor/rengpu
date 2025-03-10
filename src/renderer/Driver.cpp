#include <fstream>

// for file reading in loadShaderModule, maybe move to ResourceManager later
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>


#include <webgpu/webgpu.h>
#include <tracy/Tracy.hpp>
#include <vector>

#include "Driver.hpp"
#include "logging_macros.h"


WGPURenderPipeline RdDriver::PipelineCreate(const RdSurface& p_rdSurface, const WGPUPipelineLayout& p_pipelineLayout) {
    ZoneScoped;
	WGPUShaderModule module = ShaderModuleLoad("triangles.wgsl");

	WGPUVertexAttribute posAttribute = {
		.format = WGPUVertexFormat_Float32x3,
		.offset = 0,
		.shaderLocation = 0,
	};

	WGPUVertexAttribute colorAttribute = {
		.format = WGPUVertexFormat_Float32x3,
		.offset = 3 * sizeof(float),
		.shaderLocation = 1,
	};

	std::vector<WGPUVertexAttribute> attributes = { posAttribute, colorAttribute };

	WGPUVertexBufferLayout vertexBufferLayout = {
		.arrayStride = 6 * sizeof(float),
		.stepMode = WGPUVertexStepMode_Vertex,
		.attributeCount = 2,
		.attributes = attributes.data(),
	};

	WGPUBlendState blendState = {
        .color = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_SrcAlpha,
            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
        },
        .alpha = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_Zero,
            .dstFactor = WGPUBlendFactor_One,
        },
    };

	WGPUColorTargetState colorTargetState = {
		.nextInChain = nullptr,
		.format = p_rdSurface.format,
		.blend = &blendState,
		.writeMask = WGPUColorWriteMask_All,
	};

	WGPUFragmentState fragmentState = {
		.nextInChain = nullptr,
		.module = module,
		.entryPoint = "fs_main",
		.constantCount = 0,
		.constants = nullptr,
		.targetCount = 1,
		.targets = &colorTargetState,
	};

    WGPUDepthStencilState depthStencilState = {
        .nextInChain = nullptr,
        .format = p_rdSurface.depthTextureFormat,
        .depthWriteEnabled = true,
        .depthCompare = WGPUCompareFunction_Less,
        .stencilFront = {
            .compare = WGPUCompareFunction_Always,
            .failOp = WGPUStencilOperation_Keep,
            .depthFailOp = WGPUStencilOperation_Keep,
            .passOp = WGPUStencilOperation_Keep,
        },
        .stencilBack = {
            .compare = WGPUCompareFunction_Always,
            .failOp = WGPUStencilOperation_Keep,
            .depthFailOp = WGPUStencilOperation_Keep,
            .passOp = WGPUStencilOperation_Keep,
        },
        .stencilReadMask = 0x00,
        .stencilWriteMask = 0x00,
        .depthBias = 0,
        .depthBiasSlopeScale = 0.0f,
        .depthBiasClamp = 0.0f,
    };

	WGPURenderPipelineDescriptor pipelineDesc = {
        .nextInChain = nullptr,
        .label = "My Pipeline",
        .layout = p_pipelineLayout,
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
            .topology = WGPUPrimitiveTopology_TriangleList,
            .stripIndexFormat = WGPUIndexFormat_Undefined,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_None,
        },
        .depthStencil = &depthStencilState,
        .multisample = {
            .nextInChain = nullptr,
            .count = 1,
            .mask = ~0u,
            .alphaToCoverageEnabled = false,
        },
        .fragment = &fragmentState,  
    };

	LOG_INFO("Pipeline created");

    return wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
}


WGPUBindGroup RdDriver::BindGroupCreate(const WGPUBindGroupLayout& p_layout, const WGPUBuffer& p_buffer) {
	WGPUBindGroupEntry bindGroupEntry = {
		.nextInChain = nullptr,
		.binding = 0,
		.buffer = p_buffer,
		.offset = 0,
		.size = 4 * sizeof(float),
		.sampler = nullptr,
		.textureView = nullptr,
	};

	WGPUBindGroupDescriptor bindGroupDesc = {
		.nextInChain = nullptr,
		.label = "My Bind Group",
		.layout = p_layout,
		.entryCount = 1,
		.entries = &bindGroupEntry,
	};

    return wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
}

WGPUBindGroupLayout RdDriver::BindGroupLayoutCreate() {
	WGPUBindGroupLayoutEntry bindGroupLayoutEntry = {
        .nextInChain = nullptr,
        .binding = 0,
        .visibility = WGPUShaderStage_Vertex,
        .buffer = {
            .nextInChain = nullptr,
            .type = WGPUBufferBindingType_Uniform,

            .hasDynamicOffset = false,
            .minBindingSize = 4 * sizeof(float),
        },
        .sampler = {
            .nextInChain = nullptr,
            .type = WGPUSamplerBindingType_Undefined,
        },
        .texture = {
            .nextInChain = nullptr,
            .sampleType = WGPUTextureSampleType_Undefined,
            .viewDimension = WGPUTextureViewDimension_Undefined,
            .multisampled = false,
        },
        .storageTexture = {
            .nextInChain = nullptr,
            .access = WGPUStorageTextureAccess_Undefined,
            .format = WGPUTextureFormat_Undefined,
            .viewDimension = WGPUTextureViewDimension_Undefined,
        },
    };

	WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {
		.nextInChain = nullptr,
		.label = "My Bind Group Layout",
		.entryCount = 1,
		.entries = &bindGroupLayoutEntry,
	};

    return wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
}

WGPUShaderModule RdDriver::ShaderModuleLoad(const std::filesystem::path& filename) {
    ZoneScoped;
    std::ifstream file(std::string(RESOURCE_DIR) / filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open shader: " + filename.string());
    }
    LOG_TRACE("Shader file opened: %s", filename.c_str());
    std::ostringstream contents;
    contents << file.rdbuf();
    std::string source = contents.str();

    WGPUShaderModuleWGSLDescriptor shaderDesc = {
        .chain = {
            .next = nullptr,
            .sType = WGPUSType_ShaderModuleWGSLDescriptor,
        },
        .code = source.c_str(),
    };

    WGPUShaderModuleDescriptor moduleDesc = {
        .nextInChain = reinterpret_cast<WGPUChainedStruct*>(&shaderDesc),
        .label = filename.c_str(),
#ifdef WEBGPU_BACKEND_WGPU
        .hintCount = 0,
        .hints = nullptr,
#endif
    };

    WGPUShaderModule module = wgpuDeviceCreateShaderModule(device, &moduleDesc);
    LOG_INFO("Shader module loaded: %s", filename.c_str());

    return module;
}

bool RdDriver::GeometryLoad(
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

	enum class Section {
		None,
		Points,
		Indices,
	};
	Section currentSection = Section::None;

	float value;
	uint16_t index;
	std::string line;
	while (!file.eof()) {
		getline(file, line);

		// overcome the `CRLF` problem
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}

		if (line == "[points]") {
			currentSection = Section::Points;
		} else if (line == "[indices]") {
			currentSection = Section::Indices;
		} else if (line[0] == '#' || line.empty()) {
			// Do nothing, this is a comment
		} else if (currentSection == Section::Points) {
			std::istringstream iss(line);
			// Get x, y, z, r, g, b
            Vertex vertex;
			for (int i = 0; i < 6; ++i) {
                iss >> value;
                if (i == 0) { vertex.position.x = value; }
                if (i == 1) { vertex.position.y = value; }
                if (i == 2) { vertex.position.z = value; }
                if (i == 3) { vertex.color.r = value; }
                if (i == 4) { vertex.color.g = value; }
                if (i == 5) { vertex.color.b = value; }
			}
            vertices.push_back(vertex);

		} else if (currentSection == Section::Indices) {
			std::istringstream iss(line);
			// Get corners #0 #1 and #2
			for (int i = 0; i < 3; ++i) {
				iss >> index;
				indices.push_back(index);
			}
		}
	}
    LOG_INFO("Geometry loaded: %s", path.c_str());
	return true;
}

WGPUTextureView RdDriver::NextDepthView(const RdSurface& rdSurface) {
    ZoneScoped;

    WGPUTextureDescriptor depthDescriptor = {
        .nextInChain = nullptr,
        .label = "Depth texture",
        .usage = WGPUTextureUsage_RenderAttachment,
        .dimension = WGPUTextureDimension_2D,
        .size = {rdSurface.width, rdSurface.height, 1},
        .format = rdSurface.depthTextureFormat,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .viewFormatCount = 1,
        .viewFormats = &rdSurface.depthTextureFormat,
    };
    WGPUTexture depthTexture = wgpuDeviceCreateTexture(device, &depthDescriptor);

    WGPUTextureViewDescriptor viewDescriptor = {
        .nextInChain = nullptr,
        .label = "Depth texture view",
        .format = rdSurface.depthTextureFormat,
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = WGPUTextureAspect_DepthOnly,
    };
    WGPUTextureView depthView = wgpuTextureCreateView(depthTexture, &viewDescriptor);

	wgpuTextureRelease(depthTexture);

    return depthView;
}
