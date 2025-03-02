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
		.format = WGPUVertexFormat_Float32x2,
		.offset = 0,
		.shaderLocation = 0,
	};

	WGPUVertexAttribute colorAttribute = {
		.format = WGPUVertexFormat_Float32x3,
		.offset = 2 * sizeof(float),
		.shaderLocation = 1,
	};

	std::vector<WGPUVertexAttribute> attributes = { posAttribute, colorAttribute };

	WGPUVertexBufferLayout vertexBufferLayout = {
		.arrayStride = 5 * sizeof(float),
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
        .depthStencil = nullptr,
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

WGPUShaderModule RdDriver::ShaderModuleLoad(const std::string& filename) {
    ZoneScoped;
    std::ifstream file(std::string(RESOURCE_DIR) + filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open shader: " + filename);
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
    };

    WGPUShaderModule module = wgpuDeviceCreateShaderModule(device, &moduleDesc);
    LOG_INFO("Shader module loaded: %s", filename.c_str());

    return module;
}
