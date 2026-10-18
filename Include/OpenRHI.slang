// © 2024-2026 NVIDIA Corporation
// OpenRHI shader integration, compiled by Slang 2026.20
#ifndef OPENRHI_SLANG_INCLUDED
#define OPENRHI_SLANG_INCLUDED
#ifndef __SLANG__
#error OpenRHI shaders require Slang
#endif

#define OPENRHI_BASE_ATTRIBUTES_EMULATION_SPACE 999
#ifndef OPENRHI_SHADER_EXT_REGISTER
#define OPENRHI_SHADER_EXT_REGISTER 63
#endif

#define OPENRHI_JOIN_IMPL(a, b) a##b
#define OPENRHI_JOIN(a, b) OPENRHI_JOIN_IMPL(a, b)
#define OPENRHI_UV_TO_CLIP(uv) ((uv) * float2(2, -2) + float2(-1, 1))

#ifndef OPENRHI_TARGET_VULKAN
#define OPENRHI_TARGET_VULKAN 0
#endif
#ifndef OPENRHI_TARGET_METAL
#define OPENRHI_TARGET_METAL 0
#endif
#ifndef OPENRHI_TARGET_D3D12
#define OPENRHI_TARGET_D3D12 0
#endif

// Explicit Vulkan bindings match layouts using IGNORE_GLOBAL_SPIRV_OFFSETS
#define OPENRHI_RESOURCE(type, name, kind, index, set) \
    [[vk::binding(index, set)]] type name : register(OPENRHI_JOIN(kind, index), OPENRHI_JOIN(space, set))
#if OPENRHI_TARGET_VULKAN
#define OPENRHI_ROOT_CONSTANTS(type, name, index, set) [[vk::push_constant]] ConstantBuffer<type> name
#else
#define OPENRHI_ROOT_CONSTANTS(type, name, index, set) ConstantBuffer<type> name : register(OPENRHI_JOIN(b, index), OPENRHI_JOIN(space, set))
#endif
#define OPENRHI_FORMAT(format) [[vk::image_format(format)]]

// Indirect commands filling // TODO: change to StructuredBuffers?
#ifdef OPENRHI_USE_BYTE_ADDRESS_BUFFER
    #define OPENRHI_BUFFER_WRITE(buffer, offset, index, value) buffer.Store(offset * 4 + index * 4, value)
#else
    #define OPENRHI_BUFFER_WRITE(buffer, offset, index, value) buffer[offset + index] = value
#endif

// "DrawDesc"
#define OPENRHI_FILL_DRAW_DESC(buffer, cmdIndex, vertexNum, instanceNum, baseVertex, baseInstance) \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 4, 0, vertexNum); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 4, 1, instanceNum); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 4, 2, baseVertex); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 4, 3, baseInstance)

// see "DrawIndexedDesc"
#define OPENRHI_FILL_DRAW_INDEXED_DESC(buffer, cmdIndex, indexNum, instanceNum, baseIndex, baseVertex, baseInstance) \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 5, 0, indexNum); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 5, 1, instanceNum); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 5, 2, baseIndex); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 5, 3, baseVertex); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 5, 4, baseInstance)

// see "DrawBaseDesc"
#define OPENRHI_FILL_DRAW_BASE_DESC(buffer, cmdIndex, vertexNum, instanceNum, baseVertex, baseInstance) \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 6, 0, baseVertex); /* root constant */ \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 6, 1, baseInstance); /* root constant */ \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 6, 2, vertexNum); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 6, 3, instanceNum); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 6, 4, baseVertex); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 6, 5, baseInstance)

// see "DrawIndexedBaseDesc"
#define OPENRHI_FILL_DRAW_INDEXED_BASE_DESC(buffer, cmdIndex, indexNum, instanceNum, baseIndex, baseVertex, baseInstance) \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 7, 0, baseVertex); /* root constant */ \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 7, 1, baseInstance); /* root constant */ \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 7, 2, indexNum); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 7, 3, instanceNum); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 7, 4, baseIndex); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 7, 5, baseVertex); \
    OPENRHI_BUFFER_WRITE(buffer, cmdIndex * 7, 6, baseInstance)

#define OPENRHI_CLIP_TO_UV(clip) ((clip) * float2(0.5, -0.5) + 0.5)
#define OPENRHI_SHADING_RATE(x, y) (((x) << 2) | (y))

#if OPENRHI_TARGET_METAL
// Match the native 24-byte resource and sampler table entries
struct OpenRhiTexture2DEntry {
    uint64_t bufferAddress;
    DescriptorHandle<Texture2D<float4>> texture;
    uint64_t metadata;
};
struct OpenRhiStorageTexture2DEntry {
    uint64_t bufferAddress;
    DescriptorHandle<RWTexture2D<float4>> texture;
    uint64_t metadata;
};
struct OpenRhiSamplerEntry {
    DescriptorHandle<SamplerState> sampler;
    uint64_t unused;
    uint64_t metadata;
};
struct OpenRhiStorageBufferEntry {
    uint* data;
    uint64_t unused;
    uint64_t metadata;
};
// Fragment outputs can opt into PipelineDesc sampleMask through this constant
[[vk::constant_id(65535)]] const uint OpenRhiPipelineSampleMask = 0xffffffff;
struct OpenRhiMultiview { uint viewIndices[32]; };
struct OpenRhiAccelerationStructureHeader {
    DescriptorHandle<RaytracingAccelerationStructure> accelerationStructure;
    uint* instanceContributions;
    uint64_t reserved[6];
};
uint OpenRhiGetBufferSize(uint64_t metadata) { return uint(metadata); }
uint OpenRhiGetTypedBufferElementOffset(uint64_t metadata) { return uint(metadata >> 32) & 0xff; }
float OpenRhiGetSamplerMipBias(uint64_t metadata) { return asfloat(uint(metadata)); }
uint OpenRhiApplyPipelineSampleMask(uint mask = 0xffffffff) { return mask & OpenRhiPipelineSampleMask; }
#endif
#endif
