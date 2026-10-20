// © 2021 NVIDIA Corporation

#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
    #define OPENRHI_CALL __stdcall
#else
    #define OPENRHI_CALL
#endif

#ifndef OPENRHI_API
    #if defined(__cplusplus)
        #define OPENRHI_API extern "C"
    #else
        #define OPENRHI_API extern
    #endif
#endif

#ifdef __cplusplus
    #if !defined(OPENRHI_FORCE_C)
        #define OPENRHI_CPP
    #endif
#else
    #include <stdbool.h>
#endif

#include "OpenRHIMacro.h"

// Tips:
// - designated initializers are highly recommended!
// - always zero initialize structs via "{}" if designated initializers are not used (at least to honor "OpenRhiOptional")
// - documentation is embedded (more details can be requested by creating a GitHub issue)
// - data types are grouped into collapsible logical blocks via "#pragma region"
// - in function declarations "OpenRhiRef" implies a valid object, "OpenRhiPtr" means "NULL" is allowed

OpenRhiNamespaceBegin

// Entities
OpenRhiForwardStruct(Fence);            // a synchronization primitive that can be used to insert a dependency between queue operations or between a queue operation and the host
OpenRhiForwardStruct(Queue);            // a logical queue, providing access to a HW queue
OpenRhiForwardStruct(Memory);           // a memory blob allocated on DEVICE or HOST
OpenRhiForwardStruct(Buffer);           // a buffer object: linear arrays of data
OpenRhiForwardStruct(Device);           // a logical device
OpenRhiForwardStruct(Texture);          // a texture object: multidimensional arrays of data
OpenRhiForwardStruct(Pipeline);         // a collection of state needed for rendering: shaders + fixed
OpenRhiForwardStruct(SwapChain);        // an array of presentable images that are associated with a surface
OpenRhiForwardStruct(QueryPool);        // a collection of queries of the same type
OpenRhiForwardStruct(Descriptor);       // a handle or pointer to a resource (potentially with a header)
OpenRhiForwardStruct(CommandBuffer);    // used to record commands which can be subsequently submitted to a device queue for execution (aka command list)
OpenRhiForwardStruct(DescriptorSet);    // a continuous set of descriptors
OpenRhiForwardStruct(DescriptorPool);   // maintains a pool of descriptors, descriptor sets are allocated from
OpenRhiForwardStruct(PipelineLayout);   // determines the interface between shader stages and shader resources (aka root signature)
OpenRhiForwardStruct(PipelineCache);    // a persistent cache of compiled pipeline state objects (PSOs) to accelerate subsequent PSO creations
OpenRhiForwardStruct(CommandAllocator); // an object that command buffer memory is allocated from

// Basic types
typedef uint8_t OpenRhi(Sample_t);
typedef uint16_t OpenRhi(Dim_t);
typedef void OpenRhi(Object);

OpenRhiStruct(Uid_t) {
    uint64_t low;
    uint64_t high;
};

OpenRhiStruct(Dim2_t) {
    OpenRhi(Dim_t) w, h;
};

OpenRhiStruct(Float2_t) {
    float x, y;
};

// Aliases
static const uint32_t OpenRhiConstant(BGRA_UNUSED) = 0;     // only for "bgra" color for profiling
static const uint32_t OpenRhiConstant(ALL) = 0;             // only for "sampleMask"
static const OpenRhi(Dim_t) OpenRhiConstant(WHOLE_SIZE) = 0;    // only for "Dim_t" and "size"
static const OpenRhi(Dim_t) OpenRhiConstant(REMAINING) = 0;     // only for "mipNum" and "layerNum"

// Readability
#define OpenRhiOptional // i.e. can be 0 (keep an eye on comments)
#define OpenRhiOut      // highlights an output argument

// Implicit memory heaps for "CreatePlacedX"
#define OpenRhiDeviceHeap 0, 0
#define OpenRhiDeviceUploadHeap 0, 1
#define OpenRhiHostUploadHeap 0, 2
#define OpenRhiHostReadbackHeap 0, 3

//============================================================================================================================================================================================
#pragma region [ Common ]
//============================================================================================================================================================================================

// "AdapterDesc::supportedGraphicsAPIs" is a mask of supported graphics APIs
OpenRhiBits(GraphicsAPI, uint8_t,
    NONE    = OpenRhiBit(0), // Supports everything, does nothing, returns dummy non-NULL objects and ~0-filled descs, available if "OPENRHI_ENABLE_NONE_SUPPORT = ON" in CMake
    D3D12   = OpenRhiBit(2), // Direct3D 12 (D3D12_SDK_VERSION 4 or 619+), available if "OPENRHI_ENABLE_D3D12_SUPPORT = ON" in CMake (https://microsoft.github.io/DirectX-Specs/)
    VK      = OpenRhiBit(3), // Vulkan 1.4+, 1.3++ or 1.2+++, available if "OPENRHI_ENABLE_VK_SUPPORT = ON" in CMake (https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)
    METAL   = OpenRhiBit(5)  // Metal 4 (macOS/iOS/iPadOS 26+), available if "OPENRHI_ENABLE_METAL_SUPPORT = ON" in CMake (https://developer.apple.com/metal/)
);

OpenRhiEnum(Result, int8_t,
    // All bad, but optionally require an action ("callbackInterface.AbortExecution" is not triggered)
    DEVICE_LOST             = -3,   // may be returned by "QueueSubmit*", "*WaitIdle", "AcquireNextTexture", "QueuePresent", "WaitForPresent"
    OUT_OF_DATE             = -2,   // VK: swap chain is out of date, can be triggered if "features.resizableSwapChain" is not supported; D3D12: shader cache is stale
    INVALID_SDK             = -1,   // D3D12: some interfaces are missing (potential reasons: unable to load "D3D12Core.dll", version or SDK mismatch, developer mode is not enabled)

    // All good
    SUCCESS                 = 0,

    // All bad, most likely a crash or a validation error will happen next ("callbackInterface.AbortExecution" is triggered)
    FAILURE                 = 1,
    INVALID_ARGUMENT        = 2,
    OUT_OF_MEMORY           = 3,
    UNSUPPORTED             = 4     // if enabled, OpenRHI validation can promote some to "INVALID_ARGUMENT"
);

// The viewport origin is top-left (D3D native) by default, but can be changed to bottom-left (VK native)
// https://docs.vulkan.org/refpages/latest/refpages/source/VkViewport.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_viewport
OpenRhiStruct(Viewport) {
    float x;
    float y;
    float width;
    float height;
    float depthMin;
    float depthMax;
    bool originBottomLeft;          // expects "features.viewportOriginBottomLeft"
};

// https://docs.vulkan.org/refpages/latest/refpages/source/VkRect2D.html
OpenRhiStruct(Rect) {
    int16_t x;
    int16_t y;
    OpenRhi(Dim_t) width;
    OpenRhi(Dim_t) height;
};

OpenRhiStruct(Color32f) {
    float x, y, z, w;
};

OpenRhiStruct(Color32ui) {
    uint32_t x, y, z, w;
};

OpenRhiStruct(Color32i) {
    int32_t x, y, z, w;
};

OpenRhiStruct(DepthStencil) {
    float depth;
    uint8_t stencil;
};

OpenRhiUnion(Color) {
    OpenRhi(Color32f) f;
    OpenRhi(Color32ui) ui;
    OpenRhi(Color32i) i;
};

OpenRhiUnion(ClearValue) {
    OpenRhi(DepthStencil) depthStencil;
    OpenRhi(Color) color;
};

OpenRhiStruct(SampleLocation) {
    int8_t x, y; // [-8; 7]
};

OpenRhiStruct(BufferOffset) {
    OpenRhiPtr(Buffer) buffer;
    uint64_t offset;
};

OpenRhiStruct(DataSize) {
    const void* data;
    uint64_t size;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Formats ]
//============================================================================================================================================================================================

// https://docs.vulkan.org/refpages/latest/refpages/source/VkFormat.html
// https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
// left -> right : low -> high bits
// Expected (but not guaranteed) "FormatSupportBits" are provided, but "GetFormatSupport" should be used for querying real HW support
// To demote sRGB use the previous format, i.e. "format - 1"
//                                            STORAGE_WRITE_WITHOUT_FORMAT
//                                           STORAGE_READ_WITHOUT_FORMAT |
//                                                       VERTEX_BUFFER | |
//                                            STORAGE_BUFFER_ATOMICS | | |
//                                                  STORAGE_BUFFER | | | |
//                                                        BUFFER | | | | |
//                                         MULTISAMPLE_RESOLVE | | | | | |
//                                            MULTISAMPLE_8X | | | | | | |
//                                          MULTISAMPLE_4X | | | | | | | |
//                                        MULTISAMPLE_2X | | | | | | | | |
//                                               BLEND | | | | | | | | | |
//                          DEPTH_STENCIL_ATTACHMENT | | | | | | | | | | |
//                                COLOR_ATTACHMENT | | | | | | | | | | | |
//                       STORAGE_TEXTURE_ATOMICS | | | | | | | | | | | | |
//                             STORAGE_TEXTURE | | | | | | | | | | | | | |
//                                   TEXTURE | | | | | | | | | | | | | | |
//                                         | | | | | | | | | | | | | | | |
OpenRhiEnum(Format, uint8_t,                // |      FormatSupportBits      |
    UNKNOWN,                            // . . . . . . . . . . . . . . . .

    // Plain: 8 bits per channel
    R8_UNORM,                           // + + . + . + + + + + + + . + + +
    R8_SNORM,                           // + + . + . + + + + + + + . + + +
    R8_UINT,                            // + + . + . . + + + . + + . + + +  // "SHADING_RATE_ATTACHMENT" compatible, see "OPENRHI_SHADING_RATE" macro
    R8_SINT,                            // + + . + . . + + + . + + . + + +

    RG8_UNORM,                          // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RG8_SNORM,                          // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RG8_UINT,                           // + + . + . . + + + . + + . + + +
    RG8_SINT,                           // + + . + . . + + + . + + . + + +

    BGRA8_UNORM,                        // + + . + . + + + + + + + . + + +
    BGRA8_SRGB,                         // + . . + . + + + + + . . . . . .

    RGBA8_UNORM,                        // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RGBA8_SRGB,                         // + . . + . + + + + + . . . . . .
    RGBA8_SNORM,                        // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RGBA8_UINT,                         // + + . + . . + + + . + + . + + +
    RGBA8_SINT,                         // + + . + . . + + + . + + . + + +

    // Plain: 16 bits per channel
    R16_UNORM,                          // + + . + . + + + + + + + . + + +
    R16_SNORM,                          // + + . + . + + + + + + + . + + +
    R16_UINT,                           // + + . + . . + + + . + + . + + +
    R16_SINT,                           // + + . + . . + + + . + + . + + +
    R16_SFLOAT,                         // + + . + . + + + + + + + . + + +

    RG16_UNORM,                         // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RG16_SNORM,                         // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible
    RG16_UINT,                          // + + . + . . + + + . + + . + + +
    RG16_SINT,                          // + + . + . . + + + . + + . + + +
    RG16_SFLOAT,                        // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible

    RGBA16_UNORM,                       // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    RGBA16_SNORM,                       // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible
    RGBA16_UINT,                        // + + . + . . + + + . + + . + + +
    RGBA16_SINT,                        // + + . + . . + + + . + + . + + +
    RGBA16_SFLOAT,                      // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible

    // Plain: 32 bits per channel
    R32_UINT,                           // + + + + . . + + + . + + + + + +
    R32_SINT,                           // + + + + . . + + + . + + + + + +
    R32_SFLOAT,                         // + + + + . + + + + + + + + + + +

    RG32_UINT,                          // + + . + . . + + + . + + . + + +
    RG32_SINT,                          // + + . + . . + + + . + + . + + +
    RG32_SFLOAT,                        // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible

    RGB32_UINT,                         // + . . . . . . . . . + . . + . .
    RGB32_SINT,                         // + . . . . . . . . . + . . + . .
    RGB32_SFLOAT,                       // + . . . . . . . . + + . . + . .  // "AccelerationStructure" compatible

    RGBA32_UINT,                        // + + . + . . + + + . + + . + + +
    RGBA32_SINT,                        // + + . + . . + + + . + + . + + +
    RGBA32_SFLOAT,                      // + + . + . + + + + + + + . + + +

    // Packed: 16 bits per pixel
    B5_G6_R5_UNORM,                     // + . . + . + + + + + . . . . . .
    B5_G5_R5_A1_UNORM,                  // + . . + . + + + + + . . . . . .
    B4_G4_R4_A4_UNORM,                  // + . . . . . . . . + . . . . . .

    // Packed: 32 bits per pixel
    R10_G10_B10_A2_UNORM,               // + + . + . + + + + + + + . + + +  // "AccelerationStructure" compatible (requires "tiers.rayTracing >= 2")
    R10_G10_B10_A2_UINT,                // + + . + . . + + + . + + . + + +
    R11_G11_B10_UFLOAT,                 // + + . + . + + + + + + + . + + +
    R9_G9_B9_E5_UFLOAT,                 // + . . . . . . . . . . . . . . .

    // YUV 4:2:0 video formats
    NV12_UNORM,                         // + . . . . . . . . . . . . . . .
    P010_UNORM,                         // + . . . . . . . . . . . . . . .
    P016_UNORM,                         // + . . . . . . . . . . . . . . .

    // Block-compressed (requires "features.textureCompressionBC")
    // https://registry.khronos.org/DataFormat/specs/1.4/dataformat.1.4.html#S3TC
    // https://registry.khronos.org/DataFormat/specs/1.4/dataformat.1.4.html#RGTC
    // https://registry.khronos.org/DataFormat/specs/1.4/dataformat.1.4.html#BPTC
    BC1_RGBA_UNORM,                     // + . . . . . . . . . . . . . . .
    BC1_RGBA_SRGB,                      // + . . . . . . . . . . . . . . .
    BC2_RGBA_UNORM,                     // + . . . . . . . . . . . . . . .
    BC2_RGBA_SRGB,                      // + . . . . . . . . . . . . . . .
    BC3_RGBA_UNORM,                     // + . . . . . . . . . . . . . . .
    BC3_RGBA_SRGB,                      // + . . . . . . . . . . . . . . .
    BC4_R_UNORM,                        // + . . . . . . . . . . . . . . .
    BC4_R_SNORM,                        // + . . . . . . . . . . . . . . .
    BC5_RG_UNORM,                       // + . . . . . . . . . . . . . . .
    BC5_RG_SNORM,                       // + . . . . . . . . . . . . . . .
    BC6H_RGB_UFLOAT,                    // + . . . . . . . . . . . . . . .
    BC6H_RGB_SFLOAT,                    // + . . . . . . . . . . . . . . .
    BC7_RGBA_UNORM,                     // + . . . . . . . . . . . . . . .
    BC7_RGBA_SRGB,                      // + . . . . . . . . . . . . . . .

    // Block-compressed: Ericsson Texture Compression (requires "features.textureCompressionETC2")
    // https://registry.khronos.org/DataFormat/specs/1.4/dataformat.1.4.html#ETC2
    ETC2_RGB8_UNORM,                    // + . . . . . . . . . . . . . . .
    ETC2_RGB8_SRGB,                     // + . . . . . . . . . . . . . . .
    ETC2_RGB8_A1_UNORM,                 // + . . . . . . . . . . . . . . .
    ETC2_RGB8_A1_SRGB,                  // + . . . . . . . . . . . . . . .
    ETC2_RGB8_A8_UNORM,                 // + . . . . . . . . . . . . . . .
    ETC2_RGB8_A8_SRGB,                  // + . . . . . . . . . . . . . . .
    ETC2_R11_UNORM,                     // + . . . . . . . . . . . . . . .
    ETC2_R11_SNORM,                     // + . . . . . . . . . . . . . . .
    ETC2_R11_G11_UNORM,                 // + . . . . . . . . . . . . . . .
    ETC2_R11_G11_SNORM,                 // + . . . . . . . . . . . . . . .

    // Block-compressed: Adaptive Scalable Texture Compression (requires "features.textureCompressionASTC")
    // https://registry.khronos.org/DataFormat/specs/1.4/dataformat.1.4.html#ASTC
    ASTC_4X4_UNORM,                     // + . . . . . . . . . . . . . . .
    ASTC_4X4_SRGB,                      // + . . . . . . . . . . . . . . .
    ASTC_5X4_UNORM,                     // + . . . . . . . . . . . . . . .
    ASTC_5X4_SRGB,                      // + . . . . . . . . . . . . . . .
    ASTC_5X5_UNORM,                     // + . . . . . . . . . . . . . . .
    ASTC_5X5_SRGB,                      // + . . . . . . . . . . . . . . .
    ASTC_6X5_UNORM,                     // + . . . . . . . . . . . . . . .
    ASTC_6X5_SRGB,                      // + . . . . . . . . . . . . . . .
    ASTC_6X6_UNORM,                     // + . . . . . . . . . . . . . . .
    ASTC_6X6_SRGB,                      // + . . . . . . . . . . . . . . .
    ASTC_8X5_UNORM,                     // + . . . . . . . . . . . . . . .
    ASTC_8X5_SRGB,                      // + . . . . . . . . . . . . . . .
    ASTC_8X6_UNORM,                     // + . . . . . . . . . . . . . . .
    ASTC_8X6_SRGB,                      // + . . . . . . . . . . . . . . .
    ASTC_8X8_UNORM,                     // + . . . . . . . . . . . . . . .
    ASTC_8X8_SRGB,                      // + . . . . . . . . . . . . . . .
    ASTC_10X5_UNORM,                    // + . . . . . . . . . . . . . . .
    ASTC_10X5_SRGB,                     // + . . . . . . . . . . . . . . .
    ASTC_10X6_UNORM,                    // + . . . . . . . . . . . . . . .
    ASTC_10X6_SRGB,                     // + . . . . . . . . . . . . . . .
    ASTC_10X8_UNORM,                    // + . . . . . . . . . . . . . . .
    ASTC_10X8_SRGB,                     // + . . . . . . . . . . . . . . .
    ASTC_10X10_UNORM,                   // + . . . . . . . . . . . . . . .
    ASTC_10X10_SRGB,                    // + . . . . . . . . . . . . . . .
    ASTC_12X10_UNORM,                   // + . . . . . . . . . . . . . . .
    ASTC_12X10_SRGB,                    // + . . . . . . . . . . . . . . .
    ASTC_12X12_UNORM,                   // + . . . . . . . . . . . . . . .
    ASTC_12X12_SRGB,                    // + . . . . . . . . . . . . . . .

    // Depth
    D16_UNORM,                          // + . . . + . + + + . . . . . . .
    D32_SFLOAT,                         // + . . . + . + + + . . . . . . .

    // Depth-stencil
    D24_UNORM_S8_UINT,                  // + . . . + . + + + . . . . . . .
    D32_SFLOAT_S8_UINT                  // + . . . + . + + + . . . . . . .
);

// https://learn.microsoft.com/en-us/windows/win32/direct3d12/subresources#plane-slice
// https://docs.vulkan.org/refpages/latest/refpages/source/VkImageAspectFlagBits.html
OpenRhiBits(PlaneBits, uint8_t,
    ALL                             = 0,            // lazy default
    NONE                            = OpenRhiBit(7),    // no accessible planes (needed for a read-only depth-stencil attachment)

    COLOR                           = OpenRhiBit(0),    // indicates "color" plane (same as "ALL" for color formats)

    DEPTH                           = OpenRhiBit(1),    // indicates "depth" plane (same as "ALL" for depth-only formats)
    STENCIL                         = OpenRhiBit(2),    // indicates "stencil" plane in depth-stencil formats

    // For multi-planar YUV formats
    PLANE_0                         = OpenRhiBit(3),
    PLANE_1                         = OpenRhiBit(4),
    PLANE_2                         = OpenRhiBit(5),

    // Aliases
    PLANE_Y                         = OpenRhiMember(PlaneBits, PLANE_0),
    PLANE_UV                        = OpenRhiMember(PlaneBits, PLANE_1)
);

// A bit represents a feature, supported by a format
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_format_support
// https://docs.vulkan.org/refpages/latest/refpages/source/VkFormatFeatureFlagBits2.html
OpenRhiBits(FormatSupportBits, uint32_t,
    UNSUPPORTED                     = 0,            // format is unsupported

    // Texture
    TEXTURE                         = OpenRhiBit(0),    // sampled texture view
    STORAGE_TEXTURE                 = OpenRhiBit(1),    // storage texture view
    STORAGE_TEXTURE_ATOMICS         = OpenRhiBit(2),    // storage texture atomics other than Load / Store
    COLOR_ATTACHMENT                = OpenRhiBit(3),    // color attachment view
    DEPTH_STENCIL_ATTACHMENT        = OpenRhiBit(4),    // depth-stencil attachment view
    BLEND                           = OpenRhiBit(5),    // color attachment blending
    MULTISAMPLE_2X                  = OpenRhiBit(6),    // 2x multisampled texture
    MULTISAMPLE_4X                  = OpenRhiBit(7),    // 4x multisampled texture
    MULTISAMPLE_8X                  = OpenRhiBit(8),    // 8x multisampled texture
    MULTISAMPLE_RESOLVE             = OpenRhiBit(9),    // resolve source/destination

    // Buffer
    BUFFER                          = OpenRhiBit(10),   // typed buffer view
    STORAGE_BUFFER                  = OpenRhiBit(11),   // typed storage buffer view
    STORAGE_BUFFER_ATOMICS          = OpenRhiBit(12),   // typed storage buffer atomics other than Load / Store
    VERTEX_BUFFER                   = OpenRhiBit(13),   // vertex buffer attribute

    // Texture / buffer
    STORAGE_READ_WITHOUT_FORMAT     = OpenRhiBit(14),   // storage read with unknown format
    STORAGE_WRITE_WITHOUT_FORMAT    = OpenRhiBit(15),   // storage write with unknown format

    // Host (generally supported for non-depth/stencil formats with "TEXTURE" bit support)
    HOST_COPY                       = OpenRhiBit(16)    // synchronous host copies are supported
);

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Pipeline stages and barriers ]
//============================================================================================================================================================================================

// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html
// https://docs.vulkan.org/samples/latest/samples/performance/pipeline_barriers/README.html

// A barrier consists of two phases:
// - before (source scope, 1st synchronization scope):
//   - "AccessBits" corresponding with any relevant resource usage since the preceding barrier or the start of "QueueSubmit" scope
//   - "StagesBits" of all preceding GPU work that must be completed before executing the barrier (stages to wait before the barrier)
//   - "Layout" for textures
// - after (destination scope, 2nd synchronization scope):
//   - "AccessBits" corresponding with any relevant resource usage after the barrier completes
//   - "StagesBits" of all subsequent GPU work that must wait until the barrier execution is finished (stages to halt until the barrier is executed)
//   - "Layout" for textures
// If "features.enhancedBarriers" is not supported:
//   - https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#compatibility-with-legacy-d3d12_resource_states
//   - "AccessBits::NONE" gets mapped to "COMMON" (aka "GENERAL" access), leading to potential discrepancies with VK

// https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineStageFlagBits2.html
// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_barrier_sync
OpenRhiBits(StageBits, uint32_t,
    // Special
    ALL                             = 0,            // Lazy default for barriers                          Shader stage
    NONE                            = 0x7FFFFFFF,

    // Graphics                                     // Invoked by "CmdDraw*"
    INDEX_INPUT                     = OpenRhiBit(0),    //    Index buffer consumption
    VERTEX_SHADER                   = OpenRhiBit(1),    //    Vertex shader                                   X (required within GRAPHICS bind point)
    TESS_CONTROL_SHADER             = OpenRhiBit(2),    //    Tessellation control (hull) shader              X
    TESS_EVALUATION_SHADER          = OpenRhiBit(3),    //    Tessellation evaluation (domain) shader         X
    GEOMETRY_SHADER                 = OpenRhiBit(4),    //    Geometry shader                                 X
    TASK_SHADER                     = OpenRhiBit(5),    //    Task (amplification) shader                     X
    MESH_SHADER                     = OpenRhiBit(6),    //    Mesh shader                                     X (or required within GRAPHICS bind point)
    FRAGMENT_SHADER                 = OpenRhiBit(7),    //    Fragment (pixel) shader                         X
    DEPTH_STENCIL_ATTACHMENT        = OpenRhiBit(8),    //    Depth-stencil R/W operations
    COLOR_ATTACHMENT                = OpenRhiBit(9),    //    Color R/W operations
    SHADING_RATE_ATTACHMENT         = OpenRhiBit(10),   //    Shading rate attachment R

    // Compute                                      // Invoked by "CmdDispatch*" (not Rays)
    COMPUTE_SHADER                  = OpenRhiBit(11),   //    Compute shader                                  X (required within COMPUTE bind point)

    // Ray tracing                                  // Invoked by "CmdDispatchRays*"
    RAYGEN_SHADER                   = OpenRhiBit(12),   //    Ray generation shader                           X (required within RAY_TRACING bind point)
    MISS_SHADER                     = OpenRhiBit(13),   //    Miss shader                                     X
    INTERSECTION_SHADER             = OpenRhiBit(14),   //    Intersection shader                             X
    CLOSEST_HIT_SHADER              = OpenRhiBit(15),   //    Closest hit shader                              X
    ANY_HIT_SHADER                  = OpenRhiBit(16),   //    Any hit shader                                  X
    CALLABLE_SHADER                 = OpenRhiBit(17),   //    Callable shader                                 X
    ACCELERATION_STRUCTURE          = OpenRhiBit(18),   // Invoked by "Cmd*AccelerationStructure*" commands
    MICROMAP                        = OpenRhiBit(19),   // Invoked by "Cmd*Micromap*" commands

    // Other
    COPY                            = OpenRhiBit(20),   // Invoked by "CmdCopy*", "CmdUpload*" and "CmdReadback*"
    RESOLVE                         = OpenRhiBit(21),   // Invoked by "CmdResolveTexture"
    CLEAR_STORAGE                   = OpenRhiBit(22),   // Invoked by "CmdClearStorage"

    // Modifiers
    INDIRECT                        = OpenRhiBit(23),   // Invoked by "Indirect" commands (used in addition to other bits)

    // Host
    HOST                            = OpenRhiBit(24),   // Invoked by "UploadHostMemoryToTexture" and "ReadbackTextureToHostMemory"

    // Video
    VIDEO_DECODE                    = OpenRhiBit(25),   // Invoked by "CmdDecodeVideo"
    VIDEO_ENCODE                    = OpenRhiBit(26),   // Invoked by "CmdEncodeVideo"

    // Umbrella stages
    TESSELLATION_SHADERS            = OpenRhiMember(StageBits, TESS_CONTROL_SHADER)
                                    | OpenRhiMember(StageBits, TESS_EVALUATION_SHADER),

    MESH_SHADERS                    = OpenRhiMember(StageBits, TASK_SHADER)
                                    | OpenRhiMember(StageBits, MESH_SHADER),

    GRAPHICS_SHADERS                = OpenRhiMember(StageBits, VERTEX_SHADER)
                                    | OpenRhiMember(StageBits, TESSELLATION_SHADERS)
                                    | OpenRhiMember(StageBits, GEOMETRY_SHADER)
                                    | OpenRhiMember(StageBits, MESH_SHADERS)
                                    | OpenRhiMember(StageBits, FRAGMENT_SHADER),

    RAY_TRACING_SHADERS             = OpenRhiMember(StageBits, RAYGEN_SHADER)
                                    | OpenRhiMember(StageBits, MISS_SHADER)
                                    | OpenRhiMember(StageBits, INTERSECTION_SHADER)
                                    | OpenRhiMember(StageBits, CLOSEST_HIT_SHADER)
                                    | OpenRhiMember(StageBits, ANY_HIT_SHADER)
                                    | OpenRhiMember(StageBits, CALLABLE_SHADER),

    ALL_SHADERS                     = OpenRhiMember(StageBits, GRAPHICS_SHADERS)
                                    | OpenRhiMember(StageBits, COMPUTE_SHADER)
                                    | OpenRhiMember(StageBits, RAY_TRACING_SHADERS),

    GRAPHICS                        = OpenRhiMember(StageBits, INDEX_INPUT)
                                    | OpenRhiMember(StageBits, GRAPHICS_SHADERS)
                                    | OpenRhiMember(StageBits, DEPTH_STENCIL_ATTACHMENT)
                                    | OpenRhiMember(StageBits, COLOR_ATTACHMENT)
                                    | OpenRhiMember(StageBits, SHADING_RATE_ATTACHMENT)
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkAccessFlagBits2.html
// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_barrier_access
OpenRhiBits(AccessBits, uint32_t,
    NONE                            = 0,        // Mapped to "COMMON" (aka "GENERAL" access), if AgilitySDK is not available, leading to potential discrepancies with VK

    // Buffer                                   // Access   Compatible "StageBits" (including ALL)
    INDEX_BUFFER                    = OpenRhiBit(0),    // R        INDEX_INPUT
    VERTEX_BUFFER                   = OpenRhiBit(1),    // R        VERTEX_SHADER
    CONSTANT_BUFFER                 = OpenRhiBit(2),    // R        ALL_SHADERS
    ARGUMENT_BUFFER                 = OpenRhiBit(3),    // R        INDIRECT
    SCRATCH_BUFFER                  = OpenRhiBit(4),    // RW       ACCELERATION_STRUCTURE, MICROMAP

    // Attachment
    COLOR_ATTACHMENT_READ           = OpenRhiBit(5),    // R        COLOR_ATTACHMENT (implicitly by ROP)
    COLOR_ATTACHMENT_WRITE          = OpenRhiBit(6),    //  W       COLOR_ATTACHMENT
    DEPTH_STENCIL_ATTACHMENT_READ   = OpenRhiBit(7),    // R        DEPTH_STENCIL_ATTACHMENT
    DEPTH_STENCIL_ATTACHMENT_WRITE  = OpenRhiBit(8),    //  W       DEPTH_STENCIL_ATTACHMENT
    SHADING_RATE_ATTACHMENT         = OpenRhiBit(9),    // R        SHADING_RATE_ATTACHMENT
    INPUT_ATTACHMENT                = OpenRhiBit(10),   // R        FRAGMENT_SHADER

    // Acceleration structure
    ACCELERATION_STRUCTURE_READ     = OpenRhiBit(11),   // R        COMPUTE_SHADER, RAY_TRACING_SHADERS, ACCELERATION_STRUCTURE
    ACCELERATION_STRUCTURE_WRITE    = OpenRhiBit(12),   //  W       ACCELERATION_STRUCTURE

    // Micromap
    MICROMAP_READ                   = OpenRhiBit(13),   // R        MICROMAP, ACCELERATION_STRUCTURE
    MICROMAP_WRITE                  = OpenRhiBit(14),   //  W       MICROMAP

    // Shader
    SHADER_RESOURCE                 = OpenRhiBit(15),   // R        ALL_SHADERS
    SHADER_RESOURCE_STORAGE         = OpenRhiBit(16),   // RW       ALL_SHADERS, CLEAR_STORAGE
    SHADER_BINDING_TABLE            = OpenRhiBit(17),   // R        RAY_TRACING_SHADERS

    // Copy
    COPY_SOURCE                     = OpenRhiBit(18),   // R        COPY
    COPY_DESTINATION                = OpenRhiBit(19),   //  W       COPY

    // Resolve
    RESOLVE_SOURCE                  = OpenRhiBit(20),   // R        RESOLVE
    RESOLVE_DESTINATION             = OpenRhiBit(21),   //  W       RESOLVE

    // Clear storage
    CLEAR_STORAGE                   = OpenRhiBit(22),   //  W       CLEAR_STORAGE

    // Host
    HOST_READ                       = OpenRhiBit(23),   // R        HOST
    HOST_WRITE                      = OpenRhiBit(24),   //  W       HOST

    // Video
    VIDEO_DECODE_READ               = OpenRhiBit(25),   // R        VIDEO_DECODE
    VIDEO_DECODE_WRITE              = OpenRhiBit(26),   //  W       VIDEO_DECODE
    VIDEO_ENCODE_READ               = OpenRhiBit(27),   // R        VIDEO_ENCODE
    VIDEO_ENCODE_WRITE              = OpenRhiBit(28),   //  W       VIDEO_ENCODE
    COMMON                          = OpenRhiBit(29),   // RW       ALL; use alone, with GENERAL for textures; native interop handoff

    // Umbrella access
    COLOR_ATTACHMENT                = OpenRhiMember(AccessBits, COLOR_ATTACHMENT_READ)
                                    | OpenRhiMember(AccessBits, COLOR_ATTACHMENT_WRITE),

    DEPTH_STENCIL_ATTACHMENT        = OpenRhiMember(AccessBits, DEPTH_STENCIL_ATTACHMENT_READ)
                                    | OpenRhiMember(AccessBits, DEPTH_STENCIL_ATTACHMENT_WRITE),

    ACCELERATION_STRUCTURE          = OpenRhiMember(AccessBits, ACCELERATION_STRUCTURE_READ)
                                    | OpenRhiMember(AccessBits, ACCELERATION_STRUCTURE_WRITE),

    MICROMAP                        = OpenRhiMember(AccessBits, MICROMAP_READ)
                                    | OpenRhiMember(AccessBits, MICROMAP_WRITE),

    VIDEO_DECODE                    = OpenRhiMember(AccessBits, VIDEO_DECODE_READ)
                                    | OpenRhiMember(AccessBits, VIDEO_DECODE_WRITE),

    VIDEO_ENCODE                    = OpenRhiMember(AccessBits, VIDEO_ENCODE_READ)
                                    | OpenRhiMember(AccessBits, VIDEO_ENCODE_WRITE)
);

// "Layout" in barriers is ignored if "features.enhancedBarriers" is not supported
// https://docs.vulkan.org/refpages/latest/refpages/source/VkImageLayout.html
// https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_barrier_layout
OpenRhiEnum(Layout, uint8_t,            // Compatible "AccessBits":
    // Special
    UNDEFINED,                          // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#d3d12_barrier_layout_undefined
    GENERAL,                            // ALL access, required for "SharingMode::SIMULTANEOUS" (but may be suboptimal if "features.unifiedTextureLayouts" is not supported)
    PRESENT,                            // NONE (use "after.stages = StageBits::NONE")

    // Attachment
    COLOR_ATTACHMENT,                   // COLOR_ATTACHMENT_READ/WRITE
    DEPTH_STENCIL_ATTACHMENT,           // DEPTH_STENCIL_ATTACHMENT_READ/WRITE
    DEPTH_READONLY_STENCIL_ATTACHMENT,  // DEPTH_STENCIL_ATTACHMENT_READ/WRITE (only stencil writable), SHADER_RESOURCE (depth plane)
    DEPTH_ATTACHMENT_STENCIL_READONLY,  // DEPTH_STENCIL_ATTACHMENT_READ/WRITE (only depth writable), SHADER_RESOURCE (stencil plane)
    DEPTH_STENCIL_READONLY,             // DEPTH_STENCIL_ATTACHMENT_READ, SHADER_RESOURCE (D3D12: graphics queue only)
    SHADING_RATE_ATTACHMENT,            // SHADING_RATE_ATTACHMENT
    INPUT_ATTACHMENT,                   // COLOR_ATTACHMENT, INPUT_ATTACHMENT

    // Shader
    SHADER_RESOURCE,                    // SHADER_RESOURCE
    SHADER_RESOURCE_STORAGE,            // SHADER_RESOURCE_STORAGE

    // Copy
    COPY_SOURCE,                        // COPY_SOURCE
    COPY_DESTINATION,                   // COPY_DESTINATION

    // Resolve
    RESOLVE_SOURCE,                     // RESOLVE_SOURCE
    RESOLVE_DESTINATION,                // RESOLVE_DESTINATION

    // Video
    VIDEO_DECODE_DST,                   // VIDEO_DECODE_WRITE
    VIDEO_DECODE_DPB,                   // VIDEO_DECODE_READ/WRITE
    VIDEO_ENCODE_SRC,                   // VIDEO_ENCODE_READ
    VIDEO_ENCODE_DPB                    // VIDEO_ENCODE_READ/WRITE
);

OpenRhiStruct(AccessStage) {
    OpenRhi(AccessBits) access;
    OpenRhi(StageBits) stages;
};

OpenRhiStruct(AccessLayoutStage) {
    OpenRhi(AccessBits) access;
    OpenRhi(Layout) layout;
    OpenRhi(StageBits) stages;
};

OpenRhiStruct(GlobalBarrierDesc) {
    OpenRhi(AccessStage) before;
    OpenRhi(AccessStage) after;
};

OpenRhiStruct(BufferBarrierDesc) {
    OpenRhiPtr(Buffer) buffer;  // use "GetAccelerationStructureBuffer" and "GetMicromapBuffer" for related barriers
    OpenRhi(AccessStage) before;
    OpenRhi(AccessStage) after;
};

OpenRhiStruct(TextureBarrierDesc) {
    OpenRhiPtr(Texture) texture;
    OpenRhi(AccessLayoutStage) before;
    OpenRhi(AccessLayoutStage) after;
    OpenRhi(Dim_t) mipOffset;
    OpenRhi(Dim_t) mipNum;      // can be "REMAINING"
    OpenRhi(Dim_t) layerOffset;
    OpenRhi(Dim_t) layerNum;    // can be "REMAINING"
    OpenRhi(PlaneBits) planes;

    // Queue ownership transfer is potentially needed only for "SharingMode::EXCLUSIVE" textures
    // https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#synchronization-queue-transfers
    OpenRhiOptional OpenRhiPtr(Queue) srcQueue;
    OpenRhiOptional OpenRhiPtr(Queue) dstQueue;
};

// Using "CmdBarrier" inside a rendering pass is allowed, but only for "Layout::INPUT_ATTACHMENT" access transitions
// D3D12 filters out "transitioning to the same state" barriers if "features.enhancedBarriers" is not supported
OpenRhiStruct(BarrierDesc) {
    const OpenRhiPtr(GlobalBarrierDesc) globals;
    uint32_t globalNum;
    const OpenRhiPtr(BufferBarrierDesc) buffers;
    uint32_t bufferNum;
    const OpenRhiPtr(TextureBarrierDesc) textures;
    uint32_t textureNum;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Resources: creation ]
//============================================================================================================================================================================================

// https://docs.vulkan.org/refpages/latest/refpages/source/VkImageType.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_dimension
OpenRhiEnum(TextureType, uint8_t,
    TEXTURE_1D, // METAL: a 2D texture with "height = 1" ("texture2d" in native shaders)
    TEXTURE_2D,
    TEXTURE_3D  // arrays are unsupported
);

// OpenRHI tries to ease your life and avoid using "queue ownership transfers" (see "TextureBarrierDesc").
// In most of cases "SharingMode" can be ignored. Where is it needed?
// - VK: use "EXCLUSIVE" for attachments participating into multi-queue activities to preserve DCC (Delta Color Compression) on some HW
// - D3D12: use "SIMULTANEOUS" to concurrently use a texture as a "SHADER_RESOURCE" (or "SHADER_RESOURCE_STORAGE") and as a "COPY_DESTINATION" for non overlapping texture regions
// https://docs.vulkan.org/refpages/latest/refpages/source/VkSharingMode.html
OpenRhiEnum(SharingMode, uint8_t,
    CONCURRENT,     // VK: lazy default to avoid dealing with "queue ownership transfers", auto-optimized to "EXCLUSIVE" if all queues have the same type
    EXCLUSIVE,      // VK: may be used for attachments to preserve DCC on some HW in the cost of making a "queue ownership transfer"

    // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#single-queue-simultaneous-access
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_flags
    SIMULTANEOUS    // D3D12: strengthened variant of "CONCURRENT", allowing simultaneous multiple readers and one writer for a texture (requires "Layout::GENERAL")
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkImageUsageFlagBits.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_flags
OpenRhiBits(TextureUsageBits, uint16_t,                 // Min compatible access:                   Usage:
    NONE                                = 0,
    SHADER_RESOURCE                     = OpenRhiBit(0),    // SHADER_RESOURCE                          Read-only shader resource view (SRV)
    SHADER_RESOURCE_STORAGE             = OpenRhiBit(1),    // SHADER_RESOURCE_STORAGE                  Read/write shader resource view (UAV)
    COLOR_ATTACHMENT                    = OpenRhiBit(2),    // COLOR_ATTACHMENT                         Color attachment (render target)
    DEPTH_STENCIL_ATTACHMENT            = OpenRhiBit(3),    // DEPTH_STENCIL_ATTACHMENT_READ/WRITE      Depth-stencil attachment (depth-stencil target)
    SHADING_RATE_ATTACHMENT             = OpenRhiBit(4),    // SHADING_RATE_ATTACHMENT                  Shading rate attachment (source)
    INPUT_ATTACHMENT                    = OpenRhiBit(5),    // INPUT_ATTACHMENT                         Subpass input (read on-chip tile cache)
    HOST_TRANSFER                       = OpenRhiBit(6),    // HOST_READ/HOST_WRITE                     Synchronous copy between texture and host memory
    VIDEO_DECODE                        = OpenRhiBit(7),    // VIDEO_DECODE                             Video decode output / DPB picture
    VIDEO_ENCODE                        = OpenRhiBit(8),    // VIDEO_ENCODE                             Video encode input / DPB picture
    VIDEO_REFERENCE_ONLY                = OpenRhiBit(9)     // VIDEO_*                                  Video DPB/reference-only allocation
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkBufferUsageFlagBits.html
OpenRhiBits(BufferUsageBits, uint16_t,                  // Min compatible access:                   Usage:
    NONE                                = 0,
    SHADER_RESOURCE                     = OpenRhiBit(0),    // SHADER_RESOURCE                          Read-only shader resource view (SRV)
    SHADER_RESOURCE_STORAGE             = OpenRhiBit(1),    // SHADER_RESOURCE_STORAGE                  Read/write shader resource view (UAV)
    VERTEX                              = OpenRhiBit(2),    // VERTEX_BUFFER                            Vertex buffer
    INDEX                               = OpenRhiBit(3),    // INDEX_BUFFER                             Index buffer
    CONSTANT                            = OpenRhiBit(4),    // CONSTANT_BUFFER                          Constant buffer
    ARGUMENT                            = OpenRhiBit(5),    // ARGUMENT_BUFFER                          Argument buffer in "Indirect" commands
    SCRATCH                             = OpenRhiBit(6),    // SCRATCH_BUFFER                           Scratch buffer in "CmdBuild*" commands
    SHADER_BINDING_TABLE                = OpenRhiBit(7),    // SHADER_BINDING_TABLE                     Shader binding table (SBT) in "CmdDispatchRays*" commands
    ACCELERATION_STRUCTURE_BUILD_INPUT  = OpenRhiBit(8),    // SHADER_RESOURCE                          Read-only input in "CmdBuildAccelerationStructures" command
    ACCELERATION_STRUCTURE_STORAGE      = OpenRhiBit(9),    // ACCELERATION_STRUCTURE_READ/WRITE        (INTERNAL) acceleration structure storage
    MICROMAP_BUILD_INPUT                = OpenRhiBit(10),   // SHADER_RESOURCE                          Read-only input in "CmdBuildMicromaps" command
    MICROMAP_STORAGE                    = OpenRhiBit(11),   // MICROMAP_READ/WRITE                      (INTERNAL) micromap storage
    VIDEO_DECODE                        = OpenRhiBit(12),   // VIDEO_DECODE                             Video decode bitstream input
    VIDEO_ENCODE                        = OpenRhiBit(13)    // VIDEO_ENCODE                             Video encode bitstream output
);

OpenRhiEnum(VideoCodec, uint8_t,
    NONE,
    H264,
    H265,
    AV1
);

OpenRhiStruct(TextureDesc) {
    OpenRhi(TextureType) type;
    OpenRhi(TextureUsageBits) usage;
    OpenRhi(Format) format;
    OpenRhi(Dim_t) width;
    OpenRhiOptional OpenRhi(Dim_t) height;
    OpenRhiOptional OpenRhi(Dim_t) depth;
    OpenRhiOptional OpenRhi(Dim_t) mipNum;
    OpenRhiOptional OpenRhi(Dim_t) layerNum;
    OpenRhiOptional OpenRhi(Sample_t) sampleNum;
    OpenRhiOptional OpenRhi(SharingMode) sharingMode;
    OpenRhiOptional OpenRhi(VideoCodec) videoCodec;             // VK: required for video textures
    OpenRhiOptional OpenRhi(ClearValue) optimizedClearValue;    // D3D12: not needed on desktop, since any HW can track many clear values
};

// - VK: buffers are always created with sharing mode "CONCURRENT" to match D3D12 spec
OpenRhiStruct(BufferDesc) {
    uint64_t size;
    uint32_t structureStride;   // enable "STRUCTURED" views
    OpenRhi(BufferUsageBits) usage;
    bool byteAddress;           // enable "BYTE_ADDRESS" views
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Resources: binding to memory ]
//============================================================================================================================================================================================

// Contains some encoded implementation specific details
typedef uint32_t OpenRhi(MemoryType);

// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_type
OpenRhiEnum(MemoryLocation, uint8_t,
    DEVICE,
    DEVICE_UPLOAD, // soft fallback to "HOST_UPLOAD" if "deviceUploadHeapSize = 0"
    HOST_UPLOAD,
    HOST_READBACK
);

// Memory requirements for a resource (buffer or texture)
OpenRhiStruct(MemoryDesc) {
    uint64_t size;
    uint32_t alignment;
    OpenRhi(MemoryType) type;
    bool mustBeDedicated; // must be put into a dedicated "Memory" object, containing only 1 object with offset = 0
};

// A group of non-dedicated "MemoryDesc"s of the SAME "MemoryType" can be merged into a single memory allocation
OpenRhiStruct(AllocateMemoryDesc) {
    uint64_t size;
    OpenRhi(MemoryType) type;

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/residency
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_residency_priority
    // https://docs.vulkan.org/refpages/latest/refpages/source/VkMemoryPriorityAllocateInfoEXT.html
    float priority; // [-1; 1]: low < 0, normal = 0, high > 0

    // Memory allocation goes through "AMD Virtual Memory Allocator"
    //  - most likely a sub-allocation from a larger allocation
    //  - alignment is the maximum of all "memoryDesc.alignment" values for all resources bound to this allocation
    //  - https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    //  - https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator
    struct {
        bool enable;
        OpenRhiOptional uint32_t alignment; // by default worst-case alignment applied
    } vma;

    // If "false", may reduce alignment requirements
    bool allowMultisampleTextures;
};

// Binding resources to a memory (resources can overlap, i.e. alias)
OpenRhiStruct(BindBufferMemoryDesc) {
    OpenRhiPtr(Buffer) buffer;
    OpenRhiPtr(Memory) memory;
    uint64_t offset; // in memory
};

OpenRhiStruct(BindTextureMemoryDesc) {
    OpenRhiPtr(Texture) texture;
    OpenRhiPtr(Memory) memory;
    uint64_t offset; // in memory
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Resource views and samplers (descriptors) ]
//============================================================================================================================================================================================

// https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#creating-descriptors

OpenRhiEnum(TextureView, uint8_t,
    // Shader resources         // Slang type                        Compatible "DescriptorType"     Compatible "TextureType"
    TEXTURE,                        // Texture[1D/2D/3D](MS)            TEXTURE                         1D, 2D, 3D
    TEXTURE_ARRAY,                  // Texture[1D/2D](MS)Array          TEXTURE                         1D, 2D
    TEXTURE_CUBE,                   // TextureCube                      TEXTURE                             2D
    TEXTURE_CUBE_ARRAY,             // TextureCubeArray                 TEXTURE                             2D
    STORAGE_TEXTURE,                // RWTexture[1D/2D/3D](MS)          STORAGE_TEXTURE                 1D, 2D, 3D
    STORAGE_TEXTURE_ARRAY,          // RWTexture[1D/2D](MS)Array        STORAGE_TEXTURE                 1D, 2D
    SUBPASS_INPUT,                  // SubpassInput(MS) (non-array)     INPUT_ATTACHMENT                    2D

    // Host-only
    COLOR_ATTACHMENT,               //                                                                  1D, 2D, 3D
    DEPTH_STENCIL_ATTACHMENT,       //                                                                  1D, 2D
    SHADING_RATE_ATTACHMENT         //                                                                      2D
);

OpenRhiEnum(BufferView, uint8_t,
    // Shader resources         // Slang type                        Compatible "DescriptorType"
    BUFFER,                         // Buffer                           BUFFER
    STRUCTURED_BUFFER,              // StructuredBuffer                 STRUCTURED_BUFFER
    BYTE_ADDRESS_BUFFER,            // ByteAddressBuffer                STRUCTURED_BUFFER
    STORAGE_BUFFER,                 // RWBuffer                         STORAGE_BUFFER
    STORAGE_STRUCTURED_BUFFER,      // RWStructuredBuffer               STORAGE_STRUCTURED_BUFFER
    STORAGE_BYTE_ADDRESS_BUFFER,    // RWByteAddressBuffer              STORAGE_STRUCTURED_BUFFER
    CONSTANT_BUFFER                 // ConstantBuffer                   CONSTANT_BUFFER
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkFilter.html
// https://docs.vulkan.org/refpages/latest/refpages/source/VkSamplerMipmapMode.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_filter
OpenRhiEnum(Filter, uint8_t,
    NEAREST,
    LINEAR
);

// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_filter_reduction_type
// https://docs.vulkan.org/refpages/latest/refpages/source/VkSamplerReductionMode.html
OpenRhiEnum(FilterOp, uint8_t,
    AVERAGE,    // a weighted average (sum) of values in the footprint (default)
    MIN,        // a component-wise minimum of values in the footprint with non-zero weights, requires "features.filterOpMinMax"
    MAX         // a component-wise maximum of values in the footprint with non-zero weights, requires "features.filterOpMinMax"
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkSamplerAddressMode.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_texture_address_mode
OpenRhiEnum(AddressMode, uint8_t,
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,

    CLAMP_TO_BORDER,
    MIRROR_CLAMP_TO_EDGE
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkCompareOp.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_comparison_func
// R - fragment depth, stencil reference or "SampleCmp" reference
// D - depth or stencil buffer
OpenRhiEnum(CompareOp, uint8_t,
    NONE,                       // test is disabled
    ALWAYS,                     // true
    NEVER,                      // false
    EQUAL,                      // R == D
    NOT_EQUAL,                  // R != D
    LESS,                       // R < D
    LESS_EQUAL,                 // R <= D
    GREATER,                    // R > D
    GREATER_EQUAL               // R >= D
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkComponentSwizzle.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_shader_component_mapping
OpenRhiEnum(ComponentSwizzle, uint8_t,
    IDENTITY,                   // format-specific default

    // Requires "features.componentSwizzle"
    ZERO,                       // 0
    ONE,                        // 1 or 1.0
    R,                          // .x component (red)
    G,                          // .y component (green)
    B,                          // .z component (blue)
    A                           // .w component (alpha)
);

OpenRhiStruct(ComponentMapping) {
    // Only for non-"STORAGE" views
    OpenRhi(ComponentSwizzle) r;
    OpenRhi(ComponentSwizzle) g;
    OpenRhi(ComponentSwizzle) b;
    OpenRhi(ComponentSwizzle) a;
};

OpenRhiStruct(TextureViewDesc) {
    const OpenRhiPtr(Texture) texture;
    OpenRhi(TextureView) type;
    OpenRhi(Format) format;
    OpenRhi(Dim_t) mipOffset;
    OpenRhi(Dim_t) mipNum;                      // can be "REMAINING"
    OpenRhi(Dim_t) layerOffset;
    OpenRhi(Dim_t) layerNum;                    // can be "REMAINING"
    OpenRhi(Dim_t) sliceOffset;
    OpenRhi(Dim_t) sliceNum;                    // can be "REMAINING" (METAL: used only by "CmdClearStorage", shader views expose all slices, attachments use "sliceOffset")
    OpenRhi(PlaneBits) planes;                  // accessible planes (missing planes are read-only for "DEPTH_STENCIL_ATTACHMENT" if "layout" is not provided, i.e. "UNDEFINED")
    OpenRhi(ComponentMapping) components;

    // Overrides the expected access-time layout without transitioning the texture. May be any layout compatible with the view and texture usage, including fully or partially read-only depth-stencil layouts
    OpenRhiOptional OpenRhi(Layout) layout;         // auto-derived from the "type" and "planes" if "UNDEFINED"
};

OpenRhiStruct(BufferViewDesc) {
    const OpenRhiPtr(Buffer) buffer;
    OpenRhi(BufferView) type;
    uint64_t offset;                        // expects "memoryAlignment.bufferShaderResourceOffset" for shader resources
    uint64_t size;                          // can be "WHOLE_SIZE"
    OpenRhiOptional OpenRhi(Format) format;         // needed for typed views, i.e. "BUFFER" and "STORAGE_BUFFER"
    OpenRhiOptional uint32_t structureStride;   // needed for structured views, i.e. "STRUCTURED_BUFFER" and "STORAGE_STRUCTURED_BUFFER" (= "BufferDesc::structureStride", if not provided)
};

OpenRhiStruct(AddressModes) {
    OpenRhi(AddressMode) u, v, w;
};

OpenRhiStruct(Filters) {
    OpenRhi(Filter) min, mag, mip;
    OpenRhi(FilterOp) op;
};

// https://docs.vulkan.org/refpages/latest/refpages/source/VkSamplerCreateInfo.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_sampler_desc
OpenRhiStruct(SamplerDesc) {
    OpenRhi(Filters) filters;
    uint8_t anisotropy;
    float mipBias;                // METAL: native shaders must apply "OpenRhiGetSamplerMipBias" (0 on Apple10+, see "OpenRHI.slang")
    float mipMin;
    float mipMax;
    OpenRhi(AddressModes) addressModes;
    OpenRhi(CompareOp) compareOp;
    OpenRhi(Color) borderColor;       // used only with "AddressMode::CLAMP_TO_BORDER" (METAL: only transparent black, opaque black or opaque white)
    bool isInteger;
    bool unnormalizedCoordinates; // requires "shaderFeatures.unnormalizedCoordinates"
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Pipeline layout and descriptors management ]
//============================================================================================================================================================================================

// https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineBindPoint.html
OpenRhiEnum(BindPoint, uint8_t,
    INHERIT, // inherit from the last "CmdSetPipelineLayout" call
    GRAPHICS,
    COMPUTE,
    RAY_TRACING
);

OpenRhiBits(PipelineLayoutBits, uint8_t,
    NONE                                    = 0,
    IGNORE_GLOBAL_SPIRV_OFFSETS             = OpenRhiBit(0),    // VK: ignore "DeviceCreationDesc::vkBindingOffsets"
    ENABLE_DRAW_PARAMETERS_EMULATION        = OpenRhiBit(1),    // D3D12, METAL: enable draw parameters emulation, requires "shaderFeatures.drawParameters"
    ENABLE_DRAW_INDEX_EMULATION             = OpenRhiBit(2),    // D3D12, METAL: enable draw index emulation, requires "shaderFeatures.drawIndex"

    // Direct indexing has two modes:
    // - "descriptor pool":
    //     "MUTABLE" descriptors + "DIRECTLY_INDEXED" flags + up to two ranges in a set describing resource and sampler "virtual heaps" in a descriptor pool
    //     https://docs.shader-slang.org/en/latest/external/slang/docs/user-guide/03-convenience-features.html
    //     Match the heap descriptor sets and bindings reported by Slang reflection
    // - "descriptor heap":
    //     DescriptorHeapInterface functionality (requires "features.descriptorHeap") + no descriptor sets + at least one "DIRECTLY_INDEXED" flag
    SAMPLER_HEAP_DIRECTLY_INDEXED           = OpenRhiBit(3),    // requires "shaderModel >= 66"
    RESOURCE_HEAP_DIRECTLY_INDEXED          = OpenRhiBit(4)     // requires "shaderModel >= 66"
);

OpenRhiBits(DescriptorPoolBits, uint8_t,
    NONE                                    = 0,
    ALLOW_UPDATE_AFTER_SET                  = OpenRhiBit(0),    // allows "DescriptorSetBits::ALLOW_UPDATE_AFTER_SET"
    COPY_SOURCE                             = OpenRhiBit(1)     // allows allocated descriptor sets to be used as sources in "CopyDescriptorRanges"; such sets can't be bound
);

OpenRhiBits(DescriptorSetBits, uint8_t,
    NONE                                    = 0,
    ALLOW_UPDATE_AFTER_SET                  = OpenRhiBit(0)     // allows "DescriptorRangeBits::ALLOW_UPDATE_AFTER_SET"
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkDescriptorBindingFlagBits.html
OpenRhiBits(DescriptorRangeBits, uint8_t,
    NONE                                    = 0,

    // Requires "tiers.resourceBinding >= 1"; descriptor validity is additionally restricted by the tier
    PARTIALLY_BOUND                         = OpenRhiBit(0),    // descriptors in range may not contain valid descriptors at the time the descriptors are consumed (but referenced descriptors must be valid)
    ARRAY                                   = OpenRhiBit(1),    // descriptors in range are organized into an array

    // Requires "tiers.bindless >= 1" and "tiers.resourceBinding >= 2"
    // VK: only one range per set, resolving to the highest binding number after applying "VKBindingOffsets"
    VARIABLE_SIZED_ARRAY                    = OpenRhiBit(2),    // descriptors in range are organized into a variable-sized array, which size is specified via "variableDescriptorNum" argument of "AllocateDescriptorSets" function

    // https://docs.vulkan.org/samples/latest/samples/extensions/descriptor_indexing/README.html#_update_after_bind_streaming_descriptors_concurrently
    ALLOW_UPDATE_AFTER_SET                  = OpenRhiBit(3)     // descriptors in range can be updated after "CmdSetDescriptorSet" but before "QueueSubmit", also works as "DATA_VOLATILE"
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkDescriptorType.html
OpenRhiEnum(DescriptorType, uint8_t,
                            // Typed   Register    Compatible resources
    // Sampler heap
    SAMPLER,                    // -        s           sampler

    // Resource heap
    // - a mutable descriptor is a proxy "union" descriptor for all resource descriptor types, i.e. non-sampler
    // - a mutable descriptor can't be created, it can only be allocated from a pool (i.e. used in a "DescriptorRangeDesc")
    // - a mutable descriptor must "mutate" to any resource descriptor via "UpdateDescriptorRanges" or "CopyDescriptorRanges"
    // - a mutable descriptor range may include any non-sampler descriptors, which may be directly indexed in shaders
    MUTABLE,                    // -        -           any non-sampler

    // Optimized resources
    TEXTURE,                    // +        t           TextureView: TEXTURE, TEXTURE_ARRAY, TEXTURE_CUBE, TEXTURE_CUBE_ARRAY
    STORAGE_TEXTURE,            // +        u           TextureView: STORAGE_TEXTURE, STORAGE_TEXTURE_ARRAY
    INPUT_ATTACHMENT,           // +        -           TextureView: SUBPASS_INPUT

    BUFFER,                     // +        t           BufferView: BUFFER
    STORAGE_BUFFER,             // +        u           BufferView: STORAGE_BUFFER
    CONSTANT_BUFFER,            // -        b           BufferView: CONSTANT_BUFFER
    STRUCTURED_BUFFER,          // -        t           BufferView: STRUCTURED_BUFFER, BYTE_ADDRESS_BUFFER
    STORAGE_STRUCTURED_BUFFER,  // -        u           BufferView: STORAGE_STRUCTURED_BUFFER, STORAGE_BYTE_ADDRESS_BUFFER

    ACCELERATION_STRUCTURE      // -        t           acceleration structure, requires "features.rayTracing"
);

// "DescriptorRange" consists of "Descriptor" entities
OpenRhiStruct(DescriptorRangeDesc) {
    uint32_t baseRegisterIndex;         // "VKBindingOffsets" not applied to "MUTABLE" and "INPUT_ATTACHMENT" to avoid confusion
    uint32_t descriptorNum;             // treated as max size if "VARIABLE_SIZED_ARRAY" flag is set
    OpenRhi(DescriptorType) descriptorType;
    OpenRhi(StageBits) shaderStages;
    OpenRhi(DescriptorRangeBits) flags;
};

// "DescriptorSet" consists of "DescriptorRange" entities
OpenRhiStruct(DescriptorSetDesc) {
    uint32_t registerSpace;             // must be unique, avoid big gaps
    const OpenRhiPtr(DescriptorRangeDesc) ranges;
    uint32_t rangeNum;
    OpenRhi(DescriptorSetBits) flags;
};

// "PipelineLayout" consists of "DescriptorSet" descriptions and root parameters
OpenRhiStruct(RootConstantDesc) {           // aka push constants block
    uint32_t registerIndex;
    uint32_t size;                      // must be non-zero and a multiple of 4
    OpenRhi(StageBits) shaderStages;
};

OpenRhiStruct(RootDescriptorDesc) {         // aka push descriptor
    uint32_t registerIndex;
    OpenRhi(DescriptorType) descriptorType; // a non-typed descriptor type
    OpenRhi(StageBits) shaderStages;
};

// https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits#static-samplers
OpenRhiStruct(RootSamplerDesc) {            // aka static (immutable) sampler
    uint32_t registerIndex;
    OpenRhi(SamplerDesc) desc;
    OpenRhi(StageBits) shaderStages;
};

// https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineLayoutCreateInfo.html
// https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#root-signature
// https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#root-signature-version-11
/*
All indices are local in the currently bound pipeline layout. Pipeline layout example:
    RootConstantDesc                #0          // "rootConstantIndex" - an index in "rootConstants" in the currently bound pipeline layout
    ...

    RootDescriptorDesc              #0          // "rootDescriptorIndex" - an index in "rootDescriptors" in the currently bound pipeline layout
    ...

    RootSamplerDesc                 #0
    ...

    Descriptor set                  #0          // "setIndex" - a descriptor set index in the pipeline layout, provided as an argument or bound to the pipeline
        Descriptor range                #0      // "rangeIndex" - a descriptor range index in the descriptor set
            Descriptor num                  N   // "descriptorIndex" and "baseDescriptor" - a descriptor (base) index in the descriptor range, i.e. sub-range start
        ...
    ...
*/
OpenRhiStruct(PipelineLayoutDesc) {
    uint32_t rootRegisterSpace;         // must be unique, avoid big gaps
    const OpenRhiPtr(RootConstantDesc) rootConstants;
    uint32_t rootConstantNum;
    const OpenRhiPtr(RootDescriptorDesc) rootDescriptors;
    uint32_t rootDescriptorNum;
    const OpenRhiPtr(RootSamplerDesc) rootSamplers;
    uint32_t rootSamplerNum;
    const OpenRhiPtr(DescriptorSetDesc) descriptorSets;
    uint32_t descriptorSetNum;
    OpenRhi(StageBits) shaderStages;
    OpenRhi(PipelineLayoutBits) flags;
};

// Descriptor pool
// https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_descriptor_heap_desc
// https://docs.vulkan.org/refpages/latest/refpages/source/VkDescriptorPoolCreateInfo.html
OpenRhiStruct(DescriptorPoolDesc) {
    // Maximum number of descriptor sets that can be allocated from this pool
    uint32_t descriptorSetMaxNum;

    // Resource heap
    // - may be directly indexed in shaders via "RESOURCE_HEAP_DIRECTLY_INDEXED" pipeline layout flag
    // - https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_mutable_descriptor_type.html
    uint32_t mutableMaxNum;                 // number of "MUTABLE" descriptors, requires "features.mutableDescriptorType"

    // Sampler heap
    // - may be directly indexed in shaders via "SAMPLER_HEAP_DIRECTLY_INDEXED" pipeline layout flag
    // - root samplers do not count (not allocated from a descriptor pool)
    uint32_t samplerMaxNum;                 // number of "SAMPLER" descriptors

    // Optimized resources (may have various sizes depending on Vulkan implementation)
    uint32_t constantBufferMaxNum;          // number of "CONSTANT_BUFFER" descriptors
    uint32_t textureMaxNum;                 // number of "TEXTURE" descriptors
    uint32_t storageTextureMaxNum;          // number of "STORAGE_TEXTURE" descriptors
    uint32_t bufferMaxNum;                  // number of "BUFFER" descriptors
    uint32_t storageBufferMaxNum;           // number of "STORAGE_BUFFER" descriptors
    uint32_t structuredBufferMaxNum;        // number of "STRUCTURED_BUFFER" descriptors
    uint32_t storageStructuredBufferMaxNum; // number of "STORAGE_STRUCTURED_BUFFER" descriptors
    uint32_t accelerationStructureMaxNum;   // number of "ACCELERATION_STRUCTURE" descriptors, requires "features.rayTracing"
    uint32_t inputAttachmentMaxNum;         // number of "INPUT_ATTACHMENT" descriptors

    OpenRhi(DescriptorPoolBits) flags;
};

// Updating/initializing descriptors in a descriptor set
OpenRhiStruct(UpdateDescriptorRangeDesc) {
    // Destination
    OpenRhiPtr(DescriptorSet) descriptorSet;
    uint32_t rangeIndex;
    uint32_t baseDescriptor;
    // Source & count
    const OpenRhiPtr(Descriptor) const* descriptors; // all descriptors must have the same type
    uint32_t descriptorNum;
};

// Copying descriptors between descriptor sets
OpenRhiStruct(CopyDescriptorRangeDesc) {
    // Destination
    OpenRhiPtr(DescriptorSet) dstDescriptorSet;
    uint32_t dstRangeIndex;
    uint32_t dstBaseDescriptor;
    // Source & count
    const OpenRhiPtr(DescriptorSet) srcDescriptorSet; // must be allocated from a "DescriptorPool" with "DescriptorPoolBits::COPY_SOURCE"
    uint32_t srcRangeIndex;
    uint32_t srcBaseDescriptor;
    uint32_t descriptorNum;         // must be > 0
};

// Binding
OpenRhiStruct(SetDescriptorSetDesc) {
    uint32_t setIndex;              // an index in "PipelineLayoutDesc::descriptorSets"
    const OpenRhiPtr(DescriptorSet) descriptorSet;
    OpenRhiOptional OpenRhi(BindPoint) bindPoint;
};

OpenRhiStruct(SetRootConstantsDesc) {   // requires "pipelineLayout.rootConstantMaxSize > 0", or "descriptorHeap.rootConstantMaxSize > 0" in "descriptor heap" mode
    uint32_t rootConstantIndex;     // an index in "PipelineLayoutDesc::rootConstants"
    const void* data;
    uint32_t size;
    uint32_t offset;                // requires "features.rootConstantsOffset"
    OpenRhiOptional OpenRhi(BindPoint) bindPoint;
};

OpenRhiStruct(SetRootDescriptorDesc) {  // requires "pipelineLayout.rootDescriptorMaxNum > 0", or "descriptorHeap.rootDescriptorMaxNum > 0" in "descriptor heap" mode
    uint32_t rootDescriptorIndex;   // an index in "PipelineLayoutDesc::rootDescriptors"
    OpenRhiPtr(Descriptor) descriptor;
    uint32_t offset;                // a non-"CONSTANT_BUFFER" descriptor requires "features.nonConstantBufferRootDescriptorOffset"
    OpenRhiOptional OpenRhi(BindPoint) bindPoint;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Graphics pipeline: input assembly ]
//============================================================================================================================================================================================

OpenRhiEnum(IndexType, uint8_t,
    UINT16,
    UINT32
);

OpenRhiEnum(PrimitiveRestart, uint8_t,
    DISABLED,
    INDICES_UINT16, // index "0xFFFF" enforces primitive restart
    INDICES_UINT32  // index "0xFFFFFFFF" enforces primitive restart
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkVertexInputRate.html
OpenRhiEnum(VertexStreamStepRate, uint8_t,
    PER_VERTEX,
    PER_INSTANCE
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkPrimitiveTopology.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_primitive_topology
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_primitive_topology_type
OpenRhiEnum(Topology, uint8_t,
    POINT_LIST,
    LINE_LIST,
    LINE_STRIP,
    TRIANGLE_LIST,
    TRIANGLE_STRIP,

    // METAL: adjacency requires a geometry shader, "TRIANGLE_STRIP_WITH_ADJACENCY" is unsupported
    LINE_LIST_WITH_ADJACENCY,
    LINE_STRIP_WITH_ADJACENCY,
    TRIANGLE_LIST_WITH_ADJACENCY,
    TRIANGLE_STRIP_WITH_ADJACENCY,
    PATCH_LIST
);

OpenRhiStruct(InputAssemblyDesc) {
    OpenRhi(Topology) topology;
    uint8_t tessControlPointNum;
    OpenRhi(PrimitiveRestart) primitiveRestart;
};

OpenRhiStruct(VertexAttributeD3D) {
    const char* semanticName;
    uint32_t semanticIndex;
};

OpenRhiStruct(VertexAttributeVK) {
    uint32_t location;
};

OpenRhiStruct(VertexAttributeDesc) {
    OpenRhi(VertexAttributeD3D) d3d;
    OpenRhi(VertexAttributeVK) vk;
    uint32_t offset;
    OpenRhi(Format) format;
    uint16_t streamIndex;
};

OpenRhiStruct(VertexStreamDesc) {
    uint16_t bindingSlot;
    OpenRhi(VertexStreamStepRate) stepRate;
    OpenRhiOptional uint16_t stride; // fallback if "features.extendedDynamicState" is not supported
};

OpenRhiStruct(VertexInputDesc) {
    const OpenRhiPtr(VertexAttributeDesc) attributes;
    uint8_t attributeNum;
    const OpenRhiPtr(VertexStreamDesc) streams;
    uint8_t streamNum;
};

OpenRhiStruct(VertexBufferDesc) {
    const OpenRhiPtr(Buffer) buffer;
    uint64_t offset;
    uint32_t stride; // requires "features.extendedDynamicState", ignored otherwise, use "VertexStreamDesc::stride" instead
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Graphics pipeline: rasterization ]
//============================================================================================================================================================================================

// https://docs.vulkan.org/refpages/latest/refpages/source/VkPolygonMode.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_fill_mode
OpenRhiEnum(FillMode, uint8_t,
    SOLID,
    WIREFRAME
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkCullModeFlagBits.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_cull_mode
OpenRhiEnum(CullMode, uint8_t,
    NONE,
    FRONT,
    BACK
);

// https://docs.vulkan.org/samples/latest/samples/extensions/fragment_shading_rate_dynamic/README.html
// https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html
OpenRhiEnum(ShadingRate, uint8_t,
    FRAGMENT_SIZE_1X1,
    FRAGMENT_SIZE_1X2,
    FRAGMENT_SIZE_2X1,
    FRAGMENT_SIZE_2X2,

    // Require "features.additionalShadingRates"
    FRAGMENT_SIZE_2X4,
    FRAGMENT_SIZE_4X2,
    FRAGMENT_SIZE_4X4
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkFragmentShadingRateCombinerOpKHR.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_shading_rate_combiner
//    "primitiveCombiner"      "attachmentCombiner"
// A   Pipeline shading rate    Result of Op1
// B   Primitive shading rate   Attachment shading rate
OpenRhiEnum(ShadingRateCombiner, uint8_t,
    KEEP,       // A

    // Requires "tiers.shadingRate >= 2"
    REPLACE,    // B
    MIN,        // min(A, B)
    MAX,        // max(A, B)

    // Requires "features.sumShadingRateCombiner"
    SUM         // (A + B) or (A * B)
);

/*
https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#primsrast-depthbias-computation
R - minimum resolvable difference
S - maximum slope

bias = constant * R + slopeFactor * S
if (clamp > 0)
    bias = min(bias, clamp)
else if (clamp < 0)
    bias = max(bias, clamp)

enabled if constant != 0 or slope != 0
*/
OpenRhiStruct(DepthBiasDesc) {
    float constant;
    float clamp;
    float slope;
};

OpenRhiStruct(RasterizationDesc) {
    OpenRhi(DepthBiasDesc) depthBias;
    OpenRhi(FillMode) fillMode;
    OpenRhi(CullMode) cullMode;
    bool frontCounterClockwise;
    bool depthClamp;
    bool lineSmoothing;         // requires "features.lineSmoothing"
    bool conservativeRaster;    // requires "tiers.conservativeRaster != 0"
    bool shadingRate;           // requires "tiers.shadingRate != 0", expects "CmdSetShadingRate" and optionally "RenderingDesc::shadingRate"
};

OpenRhiStruct(MultisampleDesc) {
    uint32_t sampleMask;        // can be "ALL" (METAL: native fragment shaders must apply "OpenRhiApplyPipelineSampleMask", see "OpenRHI.slang")
    OpenRhi(Sample_t) sampleNum;
    bool alphaToCoverage;
    bool sampleLocations;       // requires "tiers.sampleLocations != 0", expects "CmdSetSampleLocations"
};

OpenRhiStruct(ShadingRateDesc) {
    OpenRhi(ShadingRate) shadingRate;
    OpenRhi(ShadingRateCombiner) primitiveCombiner;
    OpenRhi(ShadingRateCombiner) attachmentCombiner;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Graphics pipeline: output merger ]
//============================================================================================================================================================================================

OpenRhiEnum(Multiview, uint8_t,
    // Destination "viewport" and/or "layer" must be set in shaders explicitly, "viewMask" for rendering can be < than the one used for pipeline creation (D3D12 style)
    FLEXIBLE,       // requires "features.flexibleMultiview"

    // View instances go to statically assigned corresponding attachment layers, "viewMask" for rendering must match the one used for pipeline creation (VK style)
    LAYER_BASED,    // requires "features.layerBasedMultiview"

    // View instances go to statically assigned corresponding viewports, "viewMask" for pipeline creation is unused
    VIEWPORT_BASED  // requires "features.viewportBasedMultiview"
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkLogicOp.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_logic_op
// S - source color 0
// D - destination color
OpenRhiEnum(LogicOp, uint8_t,
    NONE,
    CLEAR,                      // 0
    AND,                        // S & D
    AND_REVERSE,                // S & ~D
    COPY,                       // S
    AND_INVERTED,               // ~S & D
    XOR,                        // S ^ D
    OR,                         // S | D
    NOR,                        // ~(S | D)
    EQUIVALENT,                 // ~(S ^ D)
    INVERT,                     // ~D
    OR_REVERSE,                 // S | ~D
    COPY_INVERTED,              // ~S
    OR_INVERTED,                // ~S | D
    NAND,                       // ~(S & D)
    SET                         // 1
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkStencilOp.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_stencil_op
// R - reference, set by "CmdSetStencilReference"
// D - stencil buffer
OpenRhiEnum(StencilOp, uint8_t,
    KEEP,                       // D = D
    ZERO,                       // D = 0
    REPLACE,                    // D = R
    INCREMENT_AND_CLAMP,        // D = min(D++, 255)
    DECREMENT_AND_CLAMP,        // D = max(D--, 0)
    INVERT,                     // D = ~D
    INCREMENT_AND_WRAP,         // D++
    DECREMENT_AND_WRAP          // D--
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkBlendFactor.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_blend
// S0 - source color 0
// S1 - source color 1
// D - destination color
// C - blend constants, set by "CmdSetBlendConstants"
OpenRhiEnum(BlendFactor, uint8_t,   // RGB                               ALPHA
    ZERO,                       // 0                                 0
    ONE,                        // 1                                 1
    SRC_COLOR,                  // S0.r, S0.g, S0.b                  S0.a
    ONE_MINUS_SRC_COLOR,        // 1 - S0.r, 1 - S0.g, 1 - S0.b      1 - S0.a
    DST_COLOR,                  // D.r, D.g, D.b                     D.a
    ONE_MINUS_DST_COLOR,        // 1 - D.r, 1 - D.g, 1 - D.b         1 - D.a
    SRC_ALPHA,                  // S0.a                              S0.a
    ONE_MINUS_SRC_ALPHA,        // 1 - S0.a                          1 - S0.a
    DST_ALPHA,                  // D.a                               D.a
    ONE_MINUS_DST_ALPHA,        // 1 - D.a                           1 - D.a
    CONSTANT_COLOR,             // C.r, C.g, C.b                     C.a
    ONE_MINUS_CONSTANT_COLOR,   // 1 - C.r, 1 - C.g, 1 - C.b         1 - C.a
    CONSTANT_ALPHA,             // C.a                               C.a (for RGB requires "features.constantAlphaBlendFactors")
    ONE_MINUS_CONSTANT_ALPHA,   // 1 - C.a                           1 - C.a (for RGB requires "features.constantAlphaBlendFactors")
    SRC_ALPHA_SATURATE,         // min(S0.a, 1 - D.a)                1
    SRC1_COLOR,                 // S1.r, S1.g, S1.b                  S1.a
    ONE_MINUS_SRC1_COLOR,       // 1 - S1.r, 1 - S1.g, 1 - S1.b      1 - S1.a
    SRC1_ALPHA,                 // S1.a                              S1.a
    ONE_MINUS_SRC1_ALPHA        // 1 - S1.a                          1 - S1.a
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkBlendOp.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_blend_op
// S - source color
// D - destination color
// Sf - source factor, produced by "BlendFactor"
// Df - destination factor, produced by "BlendFactor"
OpenRhiEnum(BlendOp, uint8_t,
    ADD,                        // S * Sf + D * Df
    SUBTRACT,                   // S * Sf - D * Df
    REVERSE_SUBTRACT,           // D * Df - S * Sf
    MIN,                        // min(S, D)
    MAX                         // max(S, D)
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkColorComponentFlagBits.html
OpenRhiBits(ColorWriteBits, uint8_t,
    NONE    = 0,
    R       = OpenRhiBit(0),
    G       = OpenRhiBit(1),
    B       = OpenRhiBit(2),
    A       = OpenRhiBit(3),

    RGB     = OpenRhiMember(ColorWriteBits, R) // "wingdi.h" must not be included after
            | OpenRhiMember(ColorWriteBits, G)
            | OpenRhiMember(ColorWriteBits, B),

    RGBA    = OpenRhiMember(ColorWriteBits, RGB)
            | OpenRhiMember(ColorWriteBits, A)
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkStencilOpState.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_depth_stencil_desc
OpenRhiStruct(StencilDesc) {
    OpenRhi(CompareOp) compareOp; // "compareOp != NONE", expects "CmdSetStencilReference"
    OpenRhi(StencilOp) failOp;
    OpenRhi(StencilOp) passOp;
    OpenRhi(StencilOp) depthFailOp;
    uint8_t writeMask;
    uint8_t compareMask;
};

// https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineDepthStencilStateCreateInfo.html
OpenRhiStruct(DepthAttachmentDesc) {
    OpenRhi(CompareOp) compareOp;
    bool write;
    bool boundsTest; // requires "features.depthBoundsTest", expects "CmdSetDepthBounds"
};

OpenRhiStruct(StencilAttachmentDesc) {
    OpenRhi(StencilDesc) front;
    OpenRhi(StencilDesc) back; // requires "features.independentFrontAndBackStencilReferenceAndMasks" for "back.writeMask"
};

// https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineColorBlendAttachmentState.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_render_target_blend_desc
OpenRhiStruct(BlendDesc) {
    OpenRhi(BlendFactor) srcFactor;
    OpenRhi(BlendFactor) dstFactor;
    OpenRhi(BlendOp) op;
};

OpenRhiStruct(ColorAttachmentDesc) {
    OpenRhi(Format) format;
    OpenRhi(BlendDesc) colorBlend;
    OpenRhi(BlendDesc) alphaBlend;
    OpenRhi(ColorWriteBits) colorWriteMask;
    bool blendEnabled;
};

OpenRhiStruct(OutputMergerDesc) {
    const OpenRhiPtr(ColorAttachmentDesc) colors;
    uint32_t colorNum;
    OpenRhi(DepthAttachmentDesc) depth;
    OpenRhi(StencilAttachmentDesc) stencil;
    OpenRhi(Format) depthStencilFormat;
    OpenRhi(LogicOp) logicOp;                   // requires "features.logicOp"
    OpenRhiOptional uint32_t viewMask;          // if non-0, requires "viewMaxNum > 1"
    OpenRhiOptional OpenRhi(Multiview) multiview;   // if "viewMask != 0", requires "features.(xxx)Multiview"
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Pipelines ]
//============================================================================================================================================================================================

// https://docs.vulkan.org/guide/latest/robustness.html
OpenRhiEnum(Robustness, uint8_t,
    DEFAULT,        // don't care, follow device settings (VK level when used on a device)
    OFF,            // no overhead, no robust access (out-of-bounds access is not allowed)
    VK,             // minimal overhead, partial robust access
    D3D12           // moderate overhead, D3D12-level robust access (requires "VK_EXT_robustness2", soft fallback to VK mode)
);

OpenRhiBits(GraphicsPipelineBits, uint8_t,
    NONE                = 0,
    FAIL_ON_CACHE_MISS  = OpenRhiBit(0) // "CreateGraphicsPipeline" returns "FAILURE" if the pipeline is not found in the supplied cache (requires "features.pipelineCacheControl")
);

OpenRhiBits(ComputePipelineBits, uint8_t,
    NONE                = 0,
    FAIL_ON_CACHE_MISS  = OpenRhiBit(0) // "CreateComputePipeline" returns "FAILURE" if the pipeline is not found in the supplied cache (requires "features.pipelineCacheControl")
);

OpenRhiStruct(PipelineCacheDesc) {
    OpenRhiOptional const void* data; // "data = NULL" means empty cache
    OpenRhiOptional uint64_t size;
};

// It's recommended to use "OpenRHI.slang" in the shader code
OpenRhiStruct(ShaderDesc) {
    OpenRhi(StageBits) stage;
    const void* bytecode; // see "features.shaderBytecodeXXX"
    uint64_t size;
    OpenRhiOptional const char* entryPointName;

    // METAL: required for compute, mesh and task shaders; use Slang reflection dimensions, ignored otherwise
    OpenRhiOptional OpenRhi(Dim_t) threadGroupSizeX;
    OpenRhiOptional OpenRhi(Dim_t) threadGroupSizeY;
    OpenRhiOptional OpenRhi(Dim_t) threadGroupSizeZ;
    OpenRhiOptional uint32_t taskPayloadSize; // METAL: task-to-mesh payload bytes, zero for shaders without a payload
};

OpenRhiStruct(GraphicsPipelineDesc) {
    const OpenRhiPtr(PipelineLayout) pipelineLayout;
    OpenRhiOptional const OpenRhiPtr(VertexInputDesc) vertexInput;
    OpenRhi(InputAssemblyDesc) inputAssembly;
    OpenRhi(RasterizationDesc) rasterization;
    OpenRhiOptional const OpenRhiPtr(MultisampleDesc) multisample;
    OpenRhi(OutputMergerDesc) outputMerger;
    const OpenRhiPtr(ShaderDesc) shaders;
    uint32_t shaderNum;
    OpenRhi(GraphicsPipelineBits) flags;
    OpenRhi(Robustness) robustness;
    OpenRhiOptional const OpenRhiPtr(PipelineCache) cache; // uses a cached blob on a hit and stores the result on a miss
};

OpenRhiStruct(ComputePipelineDesc) {
    const OpenRhiPtr(PipelineLayout) pipelineLayout;
    OpenRhi(ShaderDesc) shader;
    OpenRhi(ComputePipelineBits) flags;
    OpenRhi(Robustness) robustness;
    OpenRhiOptional const OpenRhiPtr(PipelineCache) cache; // uses a cached blob on a hit and stores the result on a miss
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Rendering (render pass) ]
//============================================================================================================================================================================================

// https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_dynamic_rendering.html
// https://github.com/KhronosGroup/Vulkan-Docs/blob/main/proposals/VK_KHR_dynamic_rendering_local_read.adoc

// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_render_pass_beginning_access_type
// https://docs.vulkan.org/refpages/latest/refpages/source/VkAttachmentLoadOp.html
OpenRhiEnum(LoadOp, uint8_t,
    LOAD,       // loads the existing attachment contents
    CLEAR       // clears the attachment using "AttachmentDesc::clearValue"
);

// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_render_pass_ending_access_type
// https://docs.vulkan.org/refpages/latest/refpages/source/VkAttachmentStoreOp.html
OpenRhiEnum(StoreOp, uint8_t,
    STORE,      // stores the attachment contents
    DISCARD,    // makes the attachment contents undefined
    NONE        // performs no store access if the attachment is not written, otherwise acts like "DISCARD"
);

// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resolve_mode
// https://docs.vulkan.org/refpages/latest/refpages/source/VkResolveModeFlagBits.html
// Supported resolve ops are reported per path and plane, see "DeviceDesc::resolve"
OpenRhiEnum(ResolveOp, uint8_t,
    AVERAGE,    // resolves the source samples to their average value (can't be used with integer and stencil formats)
    MIN,        // resolves the source samples to their minimum value
    MAX,        // resolves the source samples to their maximum value
    SAMPLE_ZERO // takes the value of the sample 0
);

// "OpenRhiBit(ResolveOp)"
OpenRhiBits(ResolveOpBits, uint8_t,
    NONE                            = 0,
    AVERAGE                         = OpenRhiBit(0),
    MIN                             = OpenRhiBit(1),
    MAX                             = OpenRhiBit(2),
    SAMPLE_ZERO                     = OpenRhiBit(3)
);

// Supported resolve ops of a resolve path
OpenRhiStruct(ResolveOps) {
    OpenRhi(ResolveOpBits) color;           // non-integer color formats
    OpenRhi(ResolveOpBits) colorInteger;    // integer color formats
    OpenRhi(ResolveOpBits) depth;
    OpenRhi(ResolveOpBits) stencil;
};

OpenRhiStruct(AttachmentDesc) {
    OpenRhiPtr(Descriptor) descriptor;
    OpenRhi(ClearValue) clearValue;
    OpenRhi(LoadOp) loadOp;
    OpenRhi(StoreOp) storeOp;
    OpenRhi(ResolveOp) resolveOp;
    OpenRhiOptional OpenRhiPtr(Descriptor) resolveDst;          // must be in "COLOR_ATTACHMENT" state and valid during "CmdEndRendering"
};

// If "VK_KHR_dynamic_rendering" is not supported:
// - "VkRenderPass" is used under the hood
// - input attachments must be transitioned to "Layout::INPUT_ATTACHMENT" in the same command buffer before "CmdBeginRendering"
// - matching pipeline input attachment indices are inferred from these transitions
OpenRhiStruct(RenderingDesc) {
    const OpenRhiPtr(AttachmentDesc) colors;
    uint32_t colorNum;
    OpenRhi(AttachmentDesc) depth;                          // may be treated as "depth-stencil"
    OpenRhi(AttachmentDesc) stencil;                        // (optional) separation is needed for multisample resolve, otherwise "depth.resolveOp" applies to the stencil plane
    OpenRhiOptional const OpenRhiPtr(Descriptor) shadingRate;   // requires "tiers.shadingRate >= 2"
    OpenRhiOptional uint32_t viewMask;                      // if non-0, requires "viewMaxNum > 1"
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Queries ]
//============================================================================================================================================================================================

// https://microsoft.github.io/DirectX-Specs/d3d/CountersAndQueries.html
// https://docs.vulkan.org/refpages/latest/refpages/source/VkQueryType.html
OpenRhiEnum(QueryType, uint8_t,
    TIMESTAMP,                              // uint64_t, requires "features.timestamp" (for "GRAPHICS" and "COMPUTE" queues), use only "CmdEndQuery" without "CmdBeginQuery"
    TIMESTAMP_COPY_QUEUE,                   // uint64_t, requires "features.timestampCopyQueue" (for a "COPY" queue), use only "CmdEndQuery" without "CmdBeginQuery"
    OCCLUSION,                              // uint64_t, requires "features.occlusion"
    PIPELINE_STATISTICS,                    // see "PipelineStatisticsDesc", requires "features.pipelineStatistics"
    ACCELERATION_STRUCTURE_SIZE,            // uint64_t, requires "features.rayTracing"
    ACCELERATION_STRUCTURE_COMPACTED_SIZE,  // uint64_t, requires "features.rayTracing"
    MICROMAP_COMPACTED_SIZE                 // uint64_t, requires "features.micromap"
);

OpenRhiStruct(QueryPoolDesc) {
    OpenRhi(QueryType) queryType;
    uint32_t capacity;
};

// Data layout for QueryType::PIPELINE_STATISTICS
// https://docs.vulkan.org/refpages/latest/refpages/source/VkQueryPipelineStatisticFlagBits.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_query_data_pipeline_statistics
OpenRhiStruct(PipelineStatisticsDesc) {
    // Common part
    uint64_t inputVertexNum;
    uint64_t inputPrimitiveNum;
    uint64_t vertexShaderInvocationNum;
    uint64_t geometryShaderInvocationNum;
    uint64_t geometryShaderPrimitiveNum;
    uint64_t rasterizerInPrimitiveNum;
    uint64_t rasterizerOutPrimitiveNum;
    uint64_t fragmentShaderInvocationNum;
    uint64_t tessControlShaderInvocationNum;
    uint64_t tessEvaluationShaderInvocationNum;
    uint64_t computeShaderInvocationNum;

    // If "features.meshShaderPipelineStats"
    uint64_t taskShaderInvocationNum;
    uint64_t meshShaderInvocationNum;

    // D3D12: if "features.meshShaderPipelineStats"
    uint64_t meshShaderPrimitiveNum;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Command signatures ]
//============================================================================================================================================================================================

// To fill commands for indirect drawing in a shader use one of "OPENRHI_FILL_X_DESC" macros

// Command signatures (default)

OpenRhiStruct(DrawDesc) {                   // see OPENRHI_FILL_DRAW_DESC
    uint32_t vertexNum;
    uint32_t instanceNum;
    uint32_t baseVertex;                    // vertex buffer offset = CmdSetVertexBuffers.offset + baseVertex * VertexStreamDesc::stride
    uint32_t baseInstance;
};

OpenRhiStruct(DrawIndexedDesc) {            // see OPENRHI_FILL_DRAW_INDEXED_DESC
    uint32_t indexNum;
    uint32_t instanceNum;
    uint32_t baseIndex;                     // index buffer offset = CmdSetIndexBuffer.offset + baseIndex * sizeof(CmdSetIndexBuffer.indexType)
    int32_t baseVertex;                     // index += baseVertex
    uint32_t baseInstance;
};

OpenRhiStruct(DispatchDesc) {
    uint32_t workGroupNumX;
    uint32_t workGroupNumY;
    uint32_t workGroupNumZ;
};

// Modified draw command signatures, if the bound pipeline layout has "PipelineLayoutBits::ENABLE_DRAW_PARAMETERS_EMULATION"
// "PipelineLayoutBits::ENABLE_DRAW_INDEX_EMULATION" does not change the command layout

OpenRhiStruct(DrawBaseDesc) {               // see OPENRHI_FILL_DRAW_DESC
    uint32_t shaderEmulatedBaseVertex;      // root constant
    uint32_t shaderEmulatedBaseInstance;    // root constant
    uint32_t vertexNum;
    uint32_t instanceNum;
    uint32_t baseVertex;                    // vertex buffer offset = CmdSetVertexBuffers.offset + baseVertex * VertexStreamDesc::stride
    uint32_t baseInstance;
};

OpenRhiStruct(DrawIndexedBaseDesc) {        // see OPENRHI_FILL_DRAW_INDEXED_DESC
    int32_t shaderEmulatedBaseVertex;       // root constant
    uint32_t shaderEmulatedBaseInstance;    // root constant
    uint32_t indexNum;
    uint32_t instanceNum;
    uint32_t baseIndex;                     // index buffer offset = CmdSetIndexBuffer.offset + baseIndex * sizeof(CmdSetIndexBuffer.indexType)
    int32_t baseVertex;                     // index += baseVertex
    uint32_t baseInstance;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Other ]
//============================================================================================================================================================================================

// Copy
OpenRhiStruct(TextureRegionDesc) {
    OpenRhi(Dim_t) x;
    OpenRhi(Dim_t) y;
    OpenRhi(Dim_t) z;
    OpenRhi(Dim_t) width;       // can be "WHOLE_SIZE" (mip)
    OpenRhi(Dim_t) height;      // can be "WHOLE_SIZE" (mip)
    OpenRhi(Dim_t) depth;       // can be "WHOLE_SIZE" (mip)
    OpenRhi(Dim_t) mipOffset;
    OpenRhi(Dim_t) layerOffset;
    OpenRhi(PlaneBits) planes;
};

OpenRhiStruct(TextureDataLayoutDesc) {
    uint64_t offset;        // a buffer offset must be a multiple of "uploadBufferTextureSliceAlignment" (data placement alignment)
    uint32_t rowPitch;      // must be a multiple of "uploadBufferTextureRowAlignment"
    uint32_t slicePitch;    // must be a multiple of "uploadBufferTextureSliceAlignment"
};

OpenRhiStruct(UploadHostMemoryToTextureDesc) {
    const void* srcData;
    OpenRhiPtr(Texture) dstTexture;         // must be in "{AccessBits::HOST_WRITE, Layout::GENERAL, StageBits::HOST}"
    OpenRhi(TextureRegionDesc) dstRegion;
    OpenRhiOptional uint32_t srcRowPitch;   // if rows are not tightly packed
    OpenRhiOptional uint32_t srcSlicePitch; // if slices are not tightly packed
};

OpenRhiStruct(ReadbackTextureToHostMemoryDesc) {
    OpenRhiPtr(Texture) srcTexture;         // must be in "{AccessBits::HOST_READ, Layout::GENERAL, StageBits::HOST}"
    void* dstData;
    OpenRhi(TextureRegionDesc) srcRegion;
    OpenRhiOptional uint32_t dstRowPitch;   // if rows are not tightly packed
    OpenRhiOptional uint32_t dstSlicePitch; // if slices are not tightly packed
};

// Work submission
OpenRhiStruct(FenceSubmitDesc) {
    OpenRhiPtr(Fence) fence;
    uint64_t value;
    OpenRhi(StageBits) stages;
};

OpenRhiStruct(QueueSubmitDesc) {
    const OpenRhiPtr(FenceSubmitDesc) waitFences;
    uint32_t waitFenceNum;
    const OpenRhiPtr(CommandBuffer) const* commandBuffers;
    uint32_t commandBufferNum;
    const OpenRhiPtr(FenceSubmitDesc) signalFences;
    uint32_t signalFenceNum;

    // Required if "LowLatencyInterface" is enabled for the swap chain
    OpenRhiOptional const OpenRhiPtr(SwapChain) swapChain;
    OpenRhiOptional uint64_t presentId; // must match the value passed to "QueuePresent" for the frame
};

// Clear
OpenRhiStruct(ClearAttachmentDesc) {
    OpenRhi(ClearValue) value;
    OpenRhi(PlaneBits) planes;
    uint8_t colorAttachmentIndex;
};

// Required synchronization
// - variant 1: "SHADER_RESOURCE_STORAGE" access ("SHADER_RESOURCE_STORAGE" layout) and "CLEAR_STORAGE" stage + any shader stage (or "ALL")
// - variant 2: "CLEAR_STORAGE" access ("SHADER_RESOURCE_STORAGE" layout) and "CLEAR_STORAGE" stage
OpenRhiStruct(ClearStorageDesc) {
    // For any buffers and textures with integer formats:
    //  - Clears a storage descriptor with bit-precise values, copying the lower "N" bits from "value.[f/ui/i].channel"
    //    to the corresponding channel, where "N" is the number of bits in the "channel" of the resource format
    // For textures with non-integer formats:
    //  - Clears a storage descriptor with float values with format conversion from "FLOAT" to "UNORM/SNORM" where appropriate
    // For buffers:
    //  - To avoid discrepancies in behavior between GAPIs use "R32f/ui/i" formats for views
    //  - D3D: structured buffers are unsupported!
    OpenRhiPtr(Descriptor) descriptor;  // a "STORAGE" descriptor
    OpenRhi(Color) value;               // avoid overflow
    uint32_t setIndex;
    uint32_t rangeIndex;
    uint32_t descriptorIndex;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Device description and capabilities ]
//============================================================================================================================================================================================

OpenRhiEnum(Vendor, uint8_t,
    UNKNOWN,
    NVIDIA,
    AMD,
    INTEL
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceType.html
OpenRhiEnum(Architecture, uint8_t,
    UNKNOWN,
    SOFTWARE,   // CPU
    VIRTUAL,    // remote desktop?
    INTEGRATED, // UMA
    DISCRETE    // yes, please!
);

// https://docs.vulkan.org/refpages/latest/refpages/source/VkQueueFlagBits.html
// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_command_list_type
OpenRhiEnum(QueueType, uint8_t,
    GRAPHICS,
    COMPUTE,
    COPY,
    VIDEO_DECODE,
    VIDEO_ENCODE
);

// Native capability evidence used by the platform requirements and device profiles
OpenRhiBits(DeviceCapabilityBits, uint32_t,
    NONE = 0,
    D3D12_FEATURE_LEVEL_12_2 = OpenRhiBit(0),
    SHADER_MODEL_6_6 = OpenRhiBit(1),
    ENHANCED_BARRIERS = OpenRhiBit(2),
    VULKAN_1_3 = OpenRhiBit(3),
    VULKAN_1_4 = OpenRhiBit(4),
    DYNAMIC_RENDERING = OpenRhiBit(5),
    SYNCHRONIZATION_2 = OpenRhiBit(6),
    MAINTENANCE_4 = OpenRhiBit(7),
    BUFFER_DEVICE_ADDRESS = OpenRhiBit(8),
    TIMELINE_SEMAPHORE = OpenRhiBit(9),
    RUNTIME_DESCRIPTOR_ARRAY = OpenRhiBit(10),
    PARTIALLY_BOUND_DESCRIPTORS = OpenRhiBit(11),
    SAMPLED_IMAGE_NONUNIFORM_INDEXING = OpenRhiBit(12),
    METAL_4 = OpenRhiBit(13),
    APPLE_GPU_FAMILY_9 = OpenRhiBit(14),
    APPLE_GPU_FAMILY_10 = OpenRhiBit(15)
);

OpenRhiEnum(GpuFamily, uint8_t,
    UNKNOWN, AMD_RDNA_4, NVIDIA_BLACKWELL, INTEL_XE_2, INTEL_XE_3,
    APPLE_9, APPLE_10, QUALCOMM_ADRENO, ARM_MALI, SAMSUNG_XCLIPSE
);

OpenRhiEnum(ProfilePolicy, uint8_t, AUTOMATIC, GENERIC);
OpenRhiEnum(QualificationStatus, uint8_t, UNQUALIFIED, CORRECTNESS_VALIDATED, PERFORMANCE_QUALIFIED);
OpenRhiEnum(UploadPolicy, uint8_t, STAGING_COPY, UNIFIED_MEMORY);

OpenRhiStruct(DeviceRequirements) {
    OpenRhi(DeviceCapabilityBits) requiredCapabilities; // added to the platform baseline
    uint64_t minMemoryBudget;
    uint16_t minShaderModel; // D3D shader model, zero leaves the platform baseline
};

OpenRhiStruct(DeviceCompatibilityReport) {
    OpenRhi(Result) result;
    OpenRhi(GraphicsAPI) graphicsAPI;
    OpenRhi(Uid_t) adapterUid;
    OpenRhi(DeviceCapabilityBits) requiredCapabilities;
    OpenRhi(DeviceCapabilityBits) supportedCapabilities;
    OpenRhi(DeviceCapabilityBits) missingCapabilities;
    uint64_t requiredMemoryBudget;
    uint64_t availableMemoryBudget;
    bool availableMemoryBudgetKnown;
    uint16_t requiredShaderModel;
    uint16_t supportedShaderModel;
    bool backendAvailable;
    bool memoryBudgetSatisfied;
    bool shaderModelSatisfied;
};

OpenRhiStruct(DeviceProfileDesc) {
    uint32_t revision;
    OpenRhi(GpuFamily) detectedFamily;
    OpenRhi(ProfilePolicy) policy;
    OpenRhi(QualificationStatus) qualification;
    OpenRhi(UploadPolicy) uploadPolicy;
    bool tunedPoliciesEnabled;
    bool unifiedMemory;
    bool preferVulkan14;
    uint32_t recommendedGraphicsQueues;
    uint32_t recommendedComputeQueues;
    uint32_t recommendedCopyQueues;
    uint32_t recommendedWaveSize; // zero uses the shader/compiler default
};

OpenRhiStruct(AdapterDesc) {
    char name[256];
    OpenRhi(Uid_t) uid; // "LUID" (preferred) if "uid.high = 0", or "UUID" otherwise
    uint64_t videoMemorySize;
    uint64_t sharedSystemMemorySize;
    uint32_t vendorId;
    uint32_t deviceId;
    uint32_t revisionId;
    bool revisionIdKnown;
    uint64_t nativeDriverVersion;
    uint64_t memoryBudget; // available local/UMA budget at enumeration
    bool memoryBudgetKnown;
    uint32_t nativeDriverId;
    uint8_t deviceUuid[16];
    uint8_t driverUuid[16];
    uint8_t appleGpuFamily;
    char driverDescription[256];
    uint32_t driverVersion; // GAPI and OS dependent
    uint32_t queueNum[(uint32_t)OpenRhiScopedMember(QueueType, MAX_NUM)]; // per type; queues of different types may alias the same native queue
    OpenRhi(Vendor) vendor;
    OpenRhi(Architecture) architecture;
    OpenRhi(GraphicsAPI) supportedGraphicsAPIs;
};

#define OpenRhiShaderModel(major, minor) (major * 100 + minor)

// Feature support coverage: https://vulkan.gpuinfo.org/ and https://d3d12infodb.boolka.dev/
OpenRhiStruct(DeviceDesc) {
    // Common
    OpenRhi(AdapterDesc) adapterDesc; // "queueNum" reflects available number of queues per "QueueType"
    OpenRhi(GraphicsAPI) graphicsAPI;
    uint16_t openrhiVersion;
    OpenRhi(DeviceCapabilityBits) nativeCapabilities;
    uint16_t shaderModel; // see "OpenRhiShaderModel"

    // Viewport
    struct {
        uint32_t maxNum;
        int32_t boundsMin;
        int32_t boundsMax;
    } viewport;

    // Dimensions
    struct {
        uint32_t typedBufferMaxDim;
        OpenRhi(Dim_t) attachmentMaxDim;
        OpenRhi(Dim_t) attachmentLayerMaxNum;
        OpenRhi(Dim_t) texture1DMaxDim;
        OpenRhi(Dim_t) texture2DMaxDim;
        OpenRhi(Dim_t) texture3DMaxDim;
        OpenRhi(Dim_t) textureLayerMaxNum;
    } dimensions;

    // Precision bits
    struct {
        uint32_t viewportBits;
        uint32_t subPixelBits;
        uint32_t subTexelBits;
        uint32_t mipmapBits;
    } precision;

    // Memory
    struct {
        uint64_t deviceUploadHeapSize;      // ReBAR
        uint64_t bufferMaxSize;
        uint64_t allocationMaxSize;
        uint32_t allocationMaxNum;
        uint32_t samplerAllocationMaxNum;
        uint32_t constantBufferMaxRange;
        uint32_t storageBufferMaxRange;
        uint32_t bufferTextureGranularity;  // specifies a page-like granularity at which linear and non-linear resources must be placed in adjacent memory locations to avoid aliasing
        uint32_t alignmentDefault;          // (INTERNAL) worst-case alignment for a memory allocation respecting all possible placed resources, excluding multisample textures
        uint32_t alignmentMultisample;      // (INTERNAL) worst-case alignment for a memory allocation respecting all possible placed resources, including multisample textures
    } memory;

    // Memory alignment requirements
    struct {
        uint32_t uploadBufferTextureRow;
        uint32_t uploadBufferTextureSlice;
        uint32_t bufferShaderResourceOffset;
        uint32_t constantBufferOffset;
        uint32_t scratchBufferOffset;
        uint32_t shaderBindingTable;
        uint32_t accelerationStructureOffset;
        uint32_t micromapOffset;
    } memoryAlignment;

    // Pipeline layout (see "openrhiFitPipelineLayoutSettingsIntoDeviceLimits")
    // D3D12 only: "rootConstantSize" + "descriptorSetNum" * 4 + "rootDescriptorNum" * 8 + "reservedSize" <= 256, where
    // "reservedSize" is 8 bytes for "ENABLE_DRAW_PARAMETERS_EMULATION" and 4 bytes for "ENABLE_DRAW_INDEX_EMULATION"
    struct {
        uint32_t descriptorSetMaxNum;
        uint32_t rootConstantMaxSize;
        uint32_t rootDescriptorMaxNum;
        uint32_t rootSamplerMaxNum;
    } pipelineLayout;

    // Descriptor set
    struct {
        uint32_t samplerMaxNum;
        uint32_t constantBufferMaxNum;
        uint32_t storageBufferMaxNum;
        uint32_t textureMaxNum;
        uint32_t storageTextureMaxNum;

        struct {
            uint32_t samplerMaxNum;
            uint32_t constantBufferMaxNum;
            uint32_t storageBufferMaxNum;
            uint32_t textureMaxNum;
            uint32_t storageTextureMaxNum;
        } updateAfterSet;
    } descriptorSet;

    // Descriptor heap
    struct {
        uint32_t resourceMaxNum;
        uint32_t samplerMaxNum;
        uint32_t rootConstantMaxSize;
        uint32_t rootDescriptorMaxNum;
        uint32_t rootSamplerMaxNum;
    } descriptorHeap;

    // Shader stages
    struct {
        // Per stage resources
        uint32_t descriptorSamplerMaxNum;
        uint32_t descriptorConstantBufferMaxNum;
        uint32_t descriptorStorageBufferMaxNum;
        uint32_t descriptorTextureMaxNum;
        uint32_t descriptorStorageTextureMaxNum;
        uint32_t resourceMaxNum;

        struct {
            uint32_t descriptorSamplerMaxNum;
            uint32_t descriptorConstantBufferMaxNum;
            uint32_t descriptorStorageBufferMaxNum;
            uint32_t descriptorTextureMaxNum;
            uint32_t descriptorStorageTextureMaxNum;
            uint32_t resourceMaxNum;
        } updateAfterSet;

        // Vertex
        struct {
            uint32_t attributeMaxNum;
            uint32_t streamMaxNum;
            uint32_t outputComponentMaxNum;
        } vertex;

        // Tessellation control
        struct {
            float generationMaxLevel;
            uint32_t patchPointMaxNum;
            uint32_t perVertexInputComponentMaxNum;
            uint32_t perVertexOutputComponentMaxNum;
            uint32_t perPatchOutputComponentMaxNum;
            uint32_t totalOutputComponentMaxNum;
        } tesselationControl;

        // Tessellation evaluation
        struct {
            uint32_t inputComponentMaxNum;
            uint32_t outputComponentMaxNum;
        } tesselationEvaluation;

        // Geometry
        struct {
            uint32_t invocationMaxNum;
            uint32_t inputComponentMaxNum;
            uint32_t outputComponentMaxNum;
            uint32_t outputVertexMaxNum;
            uint32_t totalOutputComponentMaxNum;
        } geometry;

        // Fragment
        struct {
            uint32_t inputComponentMaxNum;
            uint32_t attachmentMaxNum;
            uint32_t dualSourceAttachmentMaxNum;
        } fragment;

        // Compute
        //  - a "dispatch" consists of "work groups" (aka "thread groups")
        //  - a "work group" consists of "waves" (aka "subgroups" or "warps")
        //  - a "wave" consists of "lanes", which can can be active, inactive or a helper:
        //    - active: the "lane" is performing its computations
        //    - inactive: the "lane" is part of the "wave" but is currently masked out
        //    - helper: these "lanes" are executed to provide auxiliary information (like derivatives) for active threads in the same 2x2 quad
        //  - "invocation" (or "thread") is a single shader instance
        //  - "lane" specifically refers to the position of a "thread" within a hardware "wave"
        //  - the concept of "wave/lane" execution applies to all shader stages
        struct {
            uint32_t dispatchMaxDim[3];
            uint32_t workGroupInvocationMaxNum;
            uint32_t workGroupMaxDim[3];
            uint32_t sharedMemoryMaxSize;
        } compute;

        // Task
        struct {
            uint32_t dispatchWorkGroupMaxNum;
            uint32_t dispatchMaxDim[3];
            uint32_t workGroupInvocationMaxNum;
            uint32_t workGroupMaxDim[3];
            uint32_t sharedMemoryMaxSize;
            uint32_t payloadMaxSize;
        } task;

        // Mesh
        struct {
            uint32_t dispatchWorkGroupMaxNum;
            uint32_t dispatchMaxDim[3];
            uint32_t workGroupInvocationMaxNum;
            uint32_t workGroupMaxDim[3];
            uint32_t sharedMemoryMaxSize;
            uint32_t outputVerticesMaxNum;
            uint32_t outputPrimitiveMaxNum;
            uint32_t outputComponentMaxNum;
        } mesh;

        // Ray tracing
        struct {
            uint32_t shaderGroupIdentifierSize;
            uint32_t shaderBindingTableMaxStride;
            uint32_t recursionMaxDepth;
        } rayTracing;
    } shaderStage;

    // Acceleration structure
    struct {
        uint64_t primitiveMaxNum; // per BLAS
        uint64_t geometryMaxNum;  // per BLAS
        uint64_t instanceMaxNum;  // per TLAS
        uint32_t micromapSubdivisionMaxLevel;
    } accelerationStructure;

    // Wave (subgroup)
    // https://github.com/microsoft/directxshadercompiler/wiki/wave-intrinsics
    // https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Derivatives.html
    struct {
        uint32_t laneMinNum;
        uint32_t laneMaxNum;
        OpenRhi(StageBits) waveOpsStages;       // SM 6.0+ (see "shaderFeatures.waveX")
        OpenRhi(StageBits) quadOpsStages;       // SM 6.0+ (see "shaderFeatures.waveQuad")
        OpenRhi(StageBits) derivativeOpsStages; // SM 6.6+ (https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Derivatives.html#derivative-functions)
    } wave;

    // Resolve (a format must also support "FormatSupportBits::MULTISAMPLE_RESOLVE")
    struct {
        OpenRhi(ResolveOps) attachment;                 // "AttachmentDesc::resolveOp"
        OpenRhi(ResolveOps) command;                    // "CmdResolveTexture"
        bool independentDepthStencil;               // depth and stencil of a depth-stencil format can be resolved with different ops
        bool independentDepthStencilNone;           // only one of depth and stencil of a depth-stencil format can be resolved
    } resolve;

    // Other
    struct {
        uint64_t timestampFrequencyHz;
        uint32_t drawIndirectMaxNum;
        float samplerLodBiasMax;
        float samplerAnisotropyMax;
        int8_t texelGatherOffsetMin;
        int8_t texelOffsetMin;
        uint8_t texelOffsetMax;
        uint8_t texelGatherOffsetMax;
        uint8_t clipDistanceMaxNum;
        uint8_t cullDistanceMaxNum;
        uint8_t combinedClipAndCullDistanceMaxNum;
        uint8_t viewMaxNum;                         // multiview is supported if > 1
        uint8_t shadingRateAttachmentTileSize;      // square size
        bool timestampCopyQueueResolveOnCopyQueue;  // if "true", "CmdCopyQueries" for "TIMESTAMP_COPY_QUEUE" requires a COPY queue, otherwise "GRAPHICS" or "COMPUTE"
    } other;

    // Tiers (0 - unsupported)
    struct {
        // https://microsoft.github.io/DirectX-Specs/d3d/ConservativeRasterization.html#tiered-support
        // 1 - 1/2 pixel uncertainty region and does not support post-snap degenerates
        // 2 - reduces the maximum uncertainty region to 1/256 and requires post-snap degenerates not be culled
        // 3 - maintains a maximum 1/256 uncertainty region and adds support for inner input coverage, aka "SV_InnerCoverage"
        uint8_t conservativeRaster;

        // https://microsoft.github.io/DirectX-Specs/d3d/ProgrammableSamplePositions.html#hardware-tiers
        // 1 - a single sample pattern can be specified to repeat for every pixel ("locationNum / sampleNum" ratio must be 1 in "CmdSetSampleLocations"),
        //     1x and 16x sample counts do not support programmable locations
        // 2 - four separate sample patterns can be specified for each pixel in a 2x2 grid ("locationNum / sampleNum" ratio can be 1 or 4 in "CmdSetSampleLocations"),
        //     all sample counts support programmable positions
        uint8_t sampleLocations;

        // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#checkfeaturesupport-structures
        // 1 - DXR 1.0: full raytracing functionality, except features below
        // 2 - DXR 1.1: adds - ray query, "CmdDispatchRaysIndirect", "GeometryIndex()" intrinsic, additional ray flags & vertex formats
        // 3 - DXR 1.2: adds - micromap, shader execution reordering
        uint8_t rayTracing;

        // https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html#feature-tiering
        // 1 - shading rate can be specified only per draw
        // 2 - adds: per primitive shading rate, per "shadingRateAttachmentTileSize" shading rate, combiners, "SV_ShadingRate" support
        uint8_t shadingRate;

        // https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#limitations-on-static-samplers
        // 0 - ALL descriptors in range must be valid by the time the command list executes
        //       GPUs: rare
        // 1 - only "CONSTANT_BUFFER" and "STORAGE" descriptors in range must be valid
        //       GPUs: NVIDIA GTX 6xx, 7xx, 9xx & 10xx series
        // 2 - only referenced descriptors must be valid
        //       GPUs: NVIDIA GTX 16xx & RTX series, AMD R9 & RX series, Intel Arc & Skylake+
        uint8_t resourceBinding;

        // Descriptor array indexing
        // 1 - unbounded arrays with dynamic indexing
        // 2 - D3D12 dynamic resources: https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html
        uint8_t bindless;

        // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_heap_tier
        // 1 - a "Memory" can support resources from all 3 categories: buffers, attachments, all other textures
        uint8_t memory;
    } tiers;

    // Features
    struct {
        // Swap chain
        bool swapChain;                                           // SwapChainInterface
        bool presentFromCompute;                                  // see "SwapChainDesc::queue"
        bool waitableSwapChain;                                   // see "SwapChainDesc::waitable"
        bool resizableSwapChain;                                  // swap chain can be resized without triggering an "OUT_OF_DATE" error

        // Multi view
        bool flexibleMultiview;                                   // see "Multiview::FLEXIBLE"
        bool layerBasedMultiview;                                 // see "Multiview::LAYRED_BASED"
        bool viewportBasedMultiview;                              // see "Multiview::VIEWPORT_BASED"

        // Texture compression
        bool textureCompressionBC;                                // all "BC" texture formats are supported
        bool textureCompressionETC2;                              // all "ETC2" texture formats are supported
        bool textureCompressionASTC;                              // all "ASTC" texture formats are supported

        // Shader bytecode
        bool shaderBytecodeDXIL;                                  // DXIL can be passed to "ShaderDesc::bytecode"
        bool shaderBytecodeSPIRV;                                 // SPIRV can be passed to "ShaderDesc::bytecode"
        bool shaderBytecodeMETALLIB;                              // Slang-generated native metallib bytecode

        // Queries
        bool occlusion;                                           // see "QueryType::OCCLUSION"
        bool timestamp;                                           // see "QueryType::TIMESTAMP"
        bool timestampCopyQueue;                                  // see "QueryType::TIMESTAMP_COPY_QUEUE", see "other.timestampCopyQueueResolveOnCopyQueue"
        bool calibratedTimestamps;                                // see "GetCalibratedTimestamps"

        // Shading rate
        bool additionalShadingRates;                              // see "ShadingRate"
        bool sumShadingRateCombiner;                              // see "ShadingRateCombiner::SUM"

        // Clear
        bool rectColorClears;                                     // see "CmdClearAttachments"
        bool rectDepthStencilClears;                              // see "CmdClearAttachments"

        // Resolve
        bool regionResolve;                                       // see "CmdResolveTexture"

        // Pipeline cache
        bool pipelineCache;                                       // "PipelineCache" support (NOP fallback if unsupported, except on error)
        bool pipelineCacheControl;                                // "FAIL_ON_CACHE_MISS" enforces "FAILURE", useful for platforms that prohibit runtime PSO compilation (e.g., Xbox GDK)

        // Vendor integration
        bool adrenoNeuralFusion;                                   // ANF temporal SR: SDK support query succeeds and required Vulkan extensions are enabled

        // Other
        bool getMemoryDesc2;                                      // "GetXxxMemoryDesc2" support (VK: requires "maintenance4", D3D: supported)
        bool enhancedBarriers;                                    // VK: supported, D3D12: requires "AgilitySDK"
        bool tessellationShader;                                  // Tessellation control and evaluation shader stages
        bool geometryShader;                                      // Geometry shader stage
        bool meshShader;                                          // MeshShaderInterface
        bool lowLatency;                                          // LowLatencyInterface
        bool descriptorHeap;                                      // DescriptorHeapInterface
        bool video;                                               // VideoInterface
        bool componentSwizzle;                                    // see "ComponentSwizzle"
        bool independentFrontAndBackStencilReferenceAndMasks;     // see "StencilAttachmentDesc::back"
        bool filterOpMinMax;                                      // see "FilterOp"
        bool constantAlphaBlendFactors;                           // see "BlendFactor::CONSTANT_ALPHA" and "BlendFactor::ONE_MINUS_CONSTANT_ALPHA"
        bool logicOp;                                             // see "LogicOp"
        bool depthBoundsTest;                                     // see "DepthAttachmentDesc::boundsTest"
        bool drawIndirectCount;                                   // see "countBuffer" and "countBufferOffset"
        bool lineSmoothing;                                       // see "RasterizationDesc::lineSmoothing"
        bool meshShaderPipelineStats;                             // see "PipelineStatisticsDesc"
        bool dynamicDepthBias;                                    // see "CmdSetDepthBias"
        bool viewportOriginBottomLeft;                            // see "Viewport"
        bool pipelineStatistics;                                  // see "QueryType::PIPELINE_STATISTICS"
        bool rootConstantsOffset;                                 // see "SetRootConstantsDesc"
        bool nonConstantBufferRootDescriptorOffset;               // see "SetRootDescriptorDesc"
        bool mutableDescriptorType;                               // see "DescriptorType::MUTABLE"
        bool extendedDynamicState;                                // VK: allows to use "VertexBufferDesc::stride" (dynamic) instead of "VertexStreamDesc::stride" (static). Widely supported
        bool unifiedTextureLayouts;                               // VK: allows to use "GENERAL" everywhere: https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_unified_image_layouts.html
        bool resourceAliasing;                                    // binding multiple distinct texture or buffer objects to overlap the same underlying memory allocation
    } features;

    // Shader features
    // https://github.com/Microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst
    struct {
        // Native types (I32 and F32 are always supported)
        // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-scalar
        bool nativeI8;                                             // "(u)int8_t"
        bool nativeI16;                                            // "(u)int16_t"
        bool nativeF16;                                            // "float16_t"
        bool nativeI64;                                            // "(u)int64_t"
        bool nativeF64;                                            // "double"

        // Atomics on native types (I32 atomics are always supported, for others it can be partial support of SMEM, texture or buffer atomics)
        // https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_Int64_and_Float_Atomics.html
        bool atomicsI16;                                           // "(u)int16_t" atomics
        bool atomicsF16;                                           // "float16_t" atomics
        bool atomicsF32;                                           // "float" atomics
        bool atomicsI64;                                           // "(u)int64_t" atomics
        bool atomicsF64;                                           // "double" atomics

        // Storage without format
        // https://learn.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads#using-unorm-and-snorm-typed-uav-loads-from-hlsl
        bool storageReadWithoutFormat;                             // OPENRHI_FORMAT("unknown") is allowed for storage reads
        bool storageWriteWithoutFormat;                            // OPENRHI_FORMAT("unknown") is allowed for storage writes

        // Wave intrinsics
        // https://github.com/microsoft/directxshadercompiler/wiki/wave-intrinsics
        bool waveQuery;                                            // WaveIsFirstLane, WaveGetLaneCount, WaveGetLaneIndex
        bool waveVote;                                             // WaveActiveAllTrue, WaveActiveAnyTrue, WaveActiveAllEqual
        bool waveShuffle;                                          // WaveReadLaneFirst, WaveReadLaneAt
        bool waveArithmetic;                                       // WaveActiveSum, WaveActiveProduct, WaveActiveMin, WaveActiveMax, WavePrefixProduct, WavePrefixSum
        bool waveReduction;                                        // WaveActiveCountBits, WaveActiveBitAnd, WaveActiveBitOr, WaveActiveBitXor, WavePrefixCountBits
        bool waveQuad;                                             // QuadReadLaneAt, QuadReadAcrossX, QuadReadAcrossY, QuadReadAcrossDiagonal

        // Other
        bool viewportIndex;                                        // SV_ViewportArrayIndex, always can be used in geometry shaders
        bool layerIndex;                                           // SV_RenderTargetArrayIndex, always can be used in geometry shaders
        bool unnormalizedCoordinates;                              // https://microsoft.github.io/DirectX-Specs/d3d/VulkanOn12.html#non-normalized-texture-sampling-coordinates
        bool clock;                                                // https://github.com/Microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst#readclock
        bool rasterizedOrderedView;                                // https://microsoft.github.io/DirectX-Specs/d3d/RasterOrderViews.html (aka fragment shader interlock)
        bool barycentric;                                          // https://github.com/microsoft/DirectXShaderCompiler/wiki/SV_Barycentrics
        bool rayTracingPositionFetch;                              // https://docs.vulkan.org/features/latest/features/proposals/VK_KHR_ray_tracing_position_fetch.html
        bool integerDotProduct;                                    // https://github.com/microsoft/DirectXShaderCompiler/wiki/Shader-Model-6.4
        bool inputAttachments;                                     // https://github.com/Microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst#subpass-inputs

        bool drawParameters;                                       // draw base parameters, native or explicitly emulated through the pipeline layout (see "SHADERS.md")
        bool drawIndex;                                            // draw index, native or explicitly emulated through the pipeline layout (see "SHADERS.md")
    } shaderFeatures;

    // Video
    struct {
        struct {
            bool H264;
            bool H265;
            bool AV1;
        } decode;

        struct {
            bool H264;
            bool H265;
            bool AV1;
        } encode;
    } videoFeatures;
};

#pragma endregion

OpenRhiNamespaceEnd
