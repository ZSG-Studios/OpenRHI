// © 2021 NVIDIA Corporation

// Goal: utilities

#pragma once

#define OPENRHI_HELPER_H 1

OpenRhiNamespaceBegin

OpenRhiStruct(VideoMemoryInfo) {
    uint64_t budgetSize;    // the OS-provided video memory budget. If "usageSize" > "budgetSize", the application may incur stuttering or performance penalties
    uint64_t usageSize;     // specifies the application’s current video memory usage
};

OpenRhiStruct(TextureSubresourceUploadDesc) {
    const void* slices;
    uint32_t sliceNum;
    uint32_t rowPitch;
    uint32_t slicePitch;
};

OpenRhiStruct(TextureUploadDesc) {
    OpenRhiOptional const OpenRhiPtr(TextureSubresourceUploadDesc) subresources; // if provided, must include ALL subresources = layerNum * mipNum
    OpenRhiPtr(Texture) texture;
    OpenRhi(AccessLayoutStage) after;
    OpenRhi(PlaneBits) planes;
};

OpenRhiStruct(BufferUploadDesc) {
    OpenRhiOptional const void* data; // if provided, must be data for the whole buffer
    OpenRhiPtr(Buffer) buffer;
    OpenRhi(AccessStage) after;
};

OpenRhiStruct(ResourceGroupDesc) {
    OpenRhi(MemoryLocation) memoryLocation;
    OpenRhiPtr(Texture) const* textures;
    uint32_t textureNum;
    OpenRhiPtr(Buffer) const* buffers;
    uint32_t bufferNum;
    OpenRhiOptional uint64_t preferredMemorySize; // desired chunk size (but can be greater if a resource doesn't fit), 256 Mb if 0
    OpenRhiOptional float residencyPriority;      // [-1; 1]: low < 0, normal = 0, high > 0
    OpenRhiOptional bool vma;                     // memory allocation goes through "AMD Virtual Memory Allocator"
};

OpenRhiStruct(FormatProps) {
    const char* name;       // format name
    OpenRhi(Format) format;     // self
    uint8_t redBits;        // R (or depth) bits
    uint8_t greenBits;      // G (or stencil) bits (0 if channels < 2)
    uint8_t blueBits;       // B bits (0 if channels < 3)
    uint8_t alphaBits;      // A (or shared exponent) bits (0 if channels < 4)
    uint8_t stride;         // block size in bytes
    uint8_t blockWidth;     // 1 for plain formats, >1 for compressed
    uint8_t blockHeight;    // 1 for plain formats, >1 for compressed
    bool isBgr;             // reversed channels (RGBA => BGRA)
    bool isCompressed;      // block-compressed format
    bool isDepth;           // has depth component
    bool isExpShared;       // shared exponent in alpha channel
    bool isFloat;           // floating point
    bool isPacked;          // 16- or 32- bit packed
    bool isInteger;         // integer
    bool isNorm;            // [0; 1] normalized
    bool isSigned;          // signed
    bool isSrgb;            // sRGB
    bool isStencil;         // has stencil component
};

// Threadsafe: yes
OpenRhiStruct(HelperInterface) {
    // Optimized memory allocation for a group of resources
    uint32_t    (OPENRHI_CALL *CalculateAllocationNumber)   (const OpenRhiRef(Device) device, const OpenRhiRef(ResourceGroupDesc) resourceGroupDesc);
    OpenRhi(Result) (OPENRHI_CALL *AllocateAndBindMemory)       (OpenRhiRef(Device) device, const OpenRhiRef(ResourceGroupDesc) resourceGroupDesc, OpenRhiOut OpenRhiPtr(Memory)* allocations); // "allocations" must have entries >= returned by "CalculateAllocationNumber"

    // Populate resources with data (not for streaming!)
    OpenRhi(Result) (OPENRHI_CALL *UploadData)                  (OpenRhiRef(Queue) queue, const OpenRhiPtr(TextureUploadDesc) textureUploadDescs, uint32_t textureUploadDescNum, const OpenRhiPtr(BufferUploadDesc) bufferUploadDescs, uint32_t bufferUploadDescNum);

    // Information about video memory
    OpenRhi(Result) (OPENRHI_CALL *QueryVideoMemoryInfo)        (const OpenRhiRef(Device) device, OpenRhi(MemoryLocation) memoryLocation, OpenRhiOut OpenRhiRef(VideoMemoryInfo) videoMemoryInfo);
};

// Format utilities
OPENRHI_API OpenRhi(Format) OPENRHI_CALL openrhiConvertDXGIFormatToOpenRHI(uint32_t dxgiFormat); // returns best-matched typed format for "TYPELESS"
OPENRHI_API OpenRhi(Format) OPENRHI_CALL openrhiConvertVKFormatToOpenRHI(uint32_t vkFormat);
OPENRHI_API uint32_t OPENRHI_CALL openrhiConvertOpenRHIFormatToDXGI(OpenRhi(Format) format);
OPENRHI_API uint32_t OPENRHI_CALL openrhiConvertOpenRHIFormatToVK(OpenRhi(Format) format);
OPENRHI_API const OpenRhiPtr(FormatProps) OPENRHI_CALL openrhiGetFormatProps(OpenRhi(Format) format);

// Strings
OPENRHI_API const char* OPENRHI_CALL openrhiGetGraphicsAPIString(OpenRhi(GraphicsAPI) graphicsAPI);

// A friendly way to get a supported depth format
static inline OpenRhi(Format) OpenRhiFunc(GetSupportedDepthFormat)(const OpenRhiRef(CoreInterface) coreInterface, const OpenRhiRef(Device) device, uint32_t minBits, bool stencil) {
    if (minBits <= 16 && !stencil) {
        if (OpenRhiDeref(coreInterface)->GetFormatSupport(device, OpenRhiScopedMember(Format, D16_UNORM)) & OpenRhiScopedMember(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
            return OpenRhiScopedMember(Format, D16_UNORM);
    }

    if (minBits <= 24) {
        if (OpenRhiDeref(coreInterface)->GetFormatSupport(device, OpenRhiScopedMember(Format, D24_UNORM_S8_UINT)) & OpenRhiScopedMember(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
            return OpenRhiScopedMember(Format, D24_UNORM_S8_UINT);
    }

    if (minBits <= 32 && !stencil) {
        if (OpenRhiDeref(coreInterface)->GetFormatSupport(device, OpenRhiScopedMember(Format, D32_SFLOAT)) & OpenRhiScopedMember(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
            return OpenRhiScopedMember(Format, D32_SFLOAT);
    }

    if (OpenRhiDeref(coreInterface)->GetFormatSupport(device, OpenRhiScopedMember(Format, D32_SFLOAT_S8_UINT)) & OpenRhiScopedMember(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
        return OpenRhiScopedMember(Format, D32_SFLOAT_S8_UINT);

    // Should be unreachable
    return OpenRhiScopedMember(Format, UNKNOWN);
}

// A convenient way to fit pipeline layout settings into the device limits, respecting various restrictions
OpenRhiStruct(PipelineLayoutSettingsDesc) {
    uint32_t descriptorSetNum;
    uint32_t descriptorRangeNum;
    uint32_t rootConstantSize;
    uint32_t rootDescriptorNum;
    uint32_t rootSamplerNum;
    bool preferRootDescriptorsOverConstants;
    bool useDescriptorHeap; // uses "DeviceDesc::descriptorHeap" limits and fits descriptor sets and ranges to 0

    // D3D12 only (see "OpenRHI.slang" for more details)
    bool enableD3D12DrawParametersEmulation;
    bool enableD3D12DrawIndexEmulation;
};

OPENRHI_API OpenRhi(PipelineLayoutSettingsDesc) OPENRHI_CALL openrhiFitPipelineLayoutSettingsIntoDeviceLimits(const OpenRhiRef(DeviceDesc) deviceDesc, const OpenRhiRef(PipelineLayoutSettingsDesc) pipelineLayoutSettingsDesc);

OpenRhiNamespaceEnd
