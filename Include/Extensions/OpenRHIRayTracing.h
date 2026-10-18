// © 2021 NVIDIA Corporation

// Goal: ray tracing
// https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html
// https://microsoft.github.io/DirectX-Specs/d3d/Raytracing2.html

#pragma once

#define OPENRHI_RAY_TRACING_H 1

OpenRhiNamespaceBegin

OpenRhiForwardStruct(AccelerationStructure); // bottom- or top- level acceleration structure (aka BLAS or TLAS respectively)
OpenRhiForwardStruct(Micromap);              // a micromap that encodes sub-triangle opacity (aka OMM, can be attached to a triangle BLAS)

static const OpenRhiPtr(Buffer) OpenRhiConstant(HAS_BUFFER) = (OpenRhiPtr(Buffer))1; // only to indicate buffer presence in "AccelerationStructureDesc"

//============================================================================================================================================================================================
#pragma region [ Pipeline ]
//============================================================================================================================================================================================

OpenRhiBits(RayTracingPipelineBits, uint8_t,
    NONE                        = 0,
    SKIP_TRIANGLES              = OpenRhiBit(0), // provides knowledge that "triangles" doesn't need to be considered
    SKIP_AABBS                  = OpenRhiBit(1), // provides knowledge that "aabbs" doesn't need to be considered
    ALLOW_MICROMAPS             = OpenRhiBit(2), // specifies that the ray tracing pipeline can be used with acceleration structures which reference micromaps
    FAIL_ON_CACHE_MISS          = OpenRhiBit(3)  // "CreateRayTracingPipeline" returns "FAILURE" if the pipeline is not found in the supplied cache (requires "features.pipelineCacheControl")
);

OpenRhiStruct(ShaderLibraryDesc) {
    const OpenRhiPtr(ShaderDesc) shaders;
    uint32_t shaderNum;
};

OpenRhiStruct(ShaderGroupDesc) {
    // Use cases:
    //  - general: RAYGEN_SHADER, MISS_SHADER or CALLABLE_SHADER
    //  - HitGroup: CLOSEST_HIT_SHADER and/or ANY_HIT_SHADER in any order
    //  - HitGroup with an intersection shader: INTERSECTION_SHADER + CLOSEST_HIT_SHADER and/or ANY_HIT_SHADER in any order
    uint32_t shaderIndices[3]; // in ShaderLibrary, starting from 1 (0 - unused)
};

OpenRhiStruct(RayTracingPipelineDesc) {
    const OpenRhiPtr(PipelineLayout) pipelineLayout;
    const OpenRhiPtr(ShaderLibraryDesc) shaderLibrary;
    const OpenRhiPtr(ShaderGroupDesc) shaderGroups;
    uint32_t shaderGroupNum;
    uint32_t recursionMaxDepth;
    uint32_t rayPayloadMaxSize;
    uint32_t rayHitAttributeMaxSize;
    OpenRhi(RayTracingPipelineBits) flags;
    OpenRhi(Robustness) robustness;
    OpenRhiOptional const OpenRhiPtr(PipelineCache) cache; // uses a cached blob on a hit and stores the result on a miss
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Opacity Micromap (OMM) ]
//============================================================================================================================================================================================

OpenRhiEnum(MicromapFormat, uint16_t,
    OPACITY_2_STATE             = 1,
    OPACITY_4_STATE             = 2
);

OpenRhiEnum(MicromapSpecialIndex, int8_t,
    // 2/4 state
    FULLY_TRANSPARENT           = -1,           // specifies that the entire triangle is fully transparent
    FULLY_OPAQUE                = -2,           // specifies that the entire triangle is fully opaque

    // 4 state
    FULLY_UNKNOWN_TRANSPARENT   = -3,           // specifies that the entire triangle is unknown-transparent
    FULLY_UNKNOWN_OPAQUE        = -4            // specifies that the entire triangle is unknown-opaque
);

OpenRhiBits(MicromapBits, uint8_t,
    NONE                        = 0,
    ALLOW_COMPACTION            = OpenRhiBit(1),    // allows to compact the micromap by copying using "COMPACT" mode
    PREFER_FAST_TRACE           = OpenRhiBit(2),    // prioritize traversal performance over build time
    PREFER_FAST_BUILD           = OpenRhiBit(3)     // prioritize build time over traversal performance
);

OpenRhiStruct(MicromapUsageDesc) {
    uint32_t triangleNum;                       // represents "MicromapTriangle" number for "{format, subdivisionLevel}" pair contained in the micromap
    uint16_t subdivisionLevel;                  // micro triangles count = 4 ^ subdivisionLevel
    OpenRhi(MicromapFormat) format;
};

OpenRhiStruct(MicromapDesc) {
    OpenRhiOptional uint64_t optimizedSize;         // can be retrieved by "CmdWriteMicromapSizes" and used for compaction via "CmdCopyMicromap"
    const OpenRhiPtr(MicromapUsageDesc) usages;
    uint32_t usageNum;
    OpenRhi(MicromapBits) flags;
};

OpenRhiStruct(BindMicromapMemoryDesc) {
    OpenRhiPtr(Micromap) micromap;
    OpenRhiPtr(Memory) memory;
    uint64_t offset;
};

OpenRhiStruct(BuildMicromapDesc) {
    OpenRhiPtr(Micromap) dst;
    const OpenRhiPtr(Buffer) dataBuffer;
    uint64_t dataOffset;
    const OpenRhiPtr(Buffer) triangleBuffer;        // contains "MicromapTriangle" entries
    uint64_t triangleOffset;
    OpenRhiPtr(Buffer) scratchBuffer;
    uint64_t scratchOffset;
};

OpenRhiStruct(BottomLevelTrianglesMicromapDesc) {
    // For each triangle in the geometry, the acceleration structure build fetches an index from "indexBuffer".
    // If an index is the unsigned cast of one of the values from "MicromapSpecialIndex" then that triangle behaves as described for that special value.
    // Otherwise that triangle uses the micromap information from "micromap" at that index plus "baseTriangle".
    // If an index buffer is not provided, "1:1" mapping between geometry triangles and micromap triangles is assumed.

    OpenRhiOptional OpenRhiPtr(Micromap) micromap;
    OpenRhiOptional const OpenRhiPtr(Buffer) indexBuffer;
    uint64_t indexOffset;
    uint32_t baseTriangle;
    OpenRhi(IndexType) indexType;
};

// Data layout
OpenRhiStruct(MicromapTriangle) {
    uint32_t dataOffset;
    uint16_t subdivisionLevel;
    OpenRhi(MicromapFormat) format;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Acceleration Structure: Bottom Level (BLAS) ]
//============================================================================================================================================================================================

OpenRhiEnum(BottomLevelGeometryType, uint8_t,
    TRIANGLES,
    AABBS
);

OpenRhiBits(BottomLevelGeometryBits, uint8_t,
    NONE                                = 0,
    OPAQUE_GEOMETRY                     = OpenRhiBit(0),    // the geometry acts as if no any hit shader is present (can be overriden by "TopLevelInstanceBits" or ray flags)
    NO_DUPLICATE_ANY_HIT_INVOCATION     = OpenRhiBit(1)     // the any-hit shader must be called once for each primitive in this geometry
);

OpenRhiStruct(BottomLevelTrianglesDesc) {
    // Vertices
    const OpenRhiPtr(Buffer) vertexBuffer;
    uint64_t vertexOffset;
    uint32_t vertexNum;
    uint16_t vertexStride;
    OpenRhi(Format) vertexFormat;

    // Indices
    OpenRhiOptional const OpenRhiPtr(Buffer) indexBuffer;
    OpenRhiOptional uint64_t indexOffset;
    OpenRhiOptional uint32_t indexNum;
    OpenRhiOptional OpenRhi(IndexType) indexType;

    // Transform
    OpenRhiOptional const OpenRhiPtr(Buffer) transformBuffer;   // contains "TransformMatrix" entries
    OpenRhiOptional uint64_t transformOffset;

    // Micromap
    OpenRhiOptional OpenRhiPtr(BottomLevelTrianglesMicromapDesc) micromap;
};

OpenRhiStruct(BottomLevelAabbsDesc) {
    const OpenRhiPtr(Buffer) buffer;                        // contains "BottomLevelAabb" entries
    uint64_t offset;
    uint32_t num;
    uint32_t stride;
};

OpenRhiStruct(BottomLevelGeometryDesc) {
    OpenRhi(BottomLevelGeometryBits) flags;
    OpenRhi(BottomLevelGeometryType) type;
    union {
        OpenRhi(BottomLevelTrianglesDesc) triangles;
        OpenRhi(BottomLevelAabbsDesc) aabbs;
    };
};

// Data layout
OpenRhiStruct(TransformMatrix) {
    float matrix[3][4]; // 3x4 row-major affine transformation matrix, the first three columns of matrix must define an invertible 3x3 matrix
};

OpenRhiStruct(BottomLevelAabb)
{
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Acceleration Structure: Top Level (TLAS) ]
//============================================================================================================================================================================================

OpenRhiBits(TopLevelInstanceBits, uint32_t,
    NONE                        = 0,
    TRIANGLE_CULL_DISABLE       = OpenRhiBit(0), // disables face culling for this instance
    TRIANGLE_FLIP_FACING        = OpenRhiBit(1), // inverts the facing determination for geometry in this instance (since the facing is determined in object space, an instance transform does not change the winding, but a geometry transform does)
    FORCE_OPAQUE                = OpenRhiBit(2), // force enable "OPAQUE_GEOMETRY" bit on all geometries referenced by this instance
    FORCE_NON_OPAQUE            = OpenRhiBit(3), // force disable "OPAQUE_GEOMETRY" bit on all geometries referenced by this instance
    FORCE_OPACITY_2_STATE       = OpenRhiBit(4), // ignore the "unknown" state and only consider the "transparent" or "opaque" bit for all 4-state micromaps encountered during traversal
    DISABLE_MICROMAPS           = OpenRhiBit(5)  // disable micromap test for all triangles and revert to using geometry opaque/non-opaque state instead
);

OpenRhiStruct(TopLevelInstance) {
    float transform[3][4];
    uint32_t instanceId                     : 24;
    uint32_t mask                           : 8;
    uint32_t shaderBindingTableRecordOffset : 24; // final hit shader binding table record index = shaderBindingTableRecordOffset + geometryIndex (from BLAS) * geometryStride (from shader) + rayOffset (from shader)
    OpenRhi(TopLevelInstanceBits) flags         : 8;
    uint64_t accelerationStructureHandle;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Acceleration structure (AS) ]
//============================================================================================================================================================================================

OpenRhiEnum(AccelerationStructureType, uint8_t,
    TOP_LEVEL,
    BOTTOM_LEVEL
);

OpenRhiBits(AccelerationStructureBits, uint8_t,
    NONE                        = 0,
    ALLOW_UPDATE                = OpenRhiBit(0),                // allows to do "updates", which are faster than "builds" (may increase memory usage, build time and decrease traversal performance)
    ALLOW_COMPACTION            = OpenRhiBit(1),                // allows to compact the acceleration structure by copying using "COMPACT" mode
    ALLOW_DATA_ACCESS           = OpenRhiBit(2),                // allows to access vertex data from shaders (requires "features.rayTracingPositionFetch")
    ALLOW_MICROMAP_UPDATE       = OpenRhiBit(3),                // allows to update micromaps via acceleration structure update (may increase size and decrease traversal performance)
    ALLOW_DISABLE_MICROMAPS     = OpenRhiBit(4),                // allows to have "DISABLE_MICROMAPS" flag for instances referencing this BLAS
    PREFER_FAST_TRACE           = OpenRhiBit(5),                // prioritize traversal performance over build time
    PREFER_FAST_BUILD           = OpenRhiBit(6),                // prioritize build time over traversal performance
    MINIMIZE_MEMORY             = OpenRhiBit(7)                 // minimize the amount of memory used during the build (may increase build time and decrease traversal performance)
);

OpenRhiStruct(AccelerationStructureDesc) {
    OpenRhiOptional uint64_t optimizedSize;                     // can be retrieved by "CmdWriteAccelerationStructureSizes" and used for compaction via "CmdCopyAccelerationStructure"
    const OpenRhiPtr(BottomLevelGeometryDesc) geometries;       // needed only for "BOTTOM_LEVEL", "HAS_BUFFER" can be used to indicate a buffer presence (no real entities needed at initialization time)
    uint32_t geometryOrInstanceNum;
    OpenRhi(AccelerationStructureBits) flags;
    OpenRhi(AccelerationStructureType) type;
};

OpenRhiStruct(BindAccelerationStructureMemoryDesc) {
    OpenRhiPtr(AccelerationStructure) accelerationStructure;
    OpenRhiPtr(Memory) memory;
    uint64_t offset;
};

OpenRhiStruct(BuildTopLevelAccelerationStructureDesc) {
    OpenRhiPtr(AccelerationStructure) dst;
    OpenRhiOptional const OpenRhiPtr(AccelerationStructure) src;    // implies "update" instead of "build" if provided (requires "ALLOW_UPDATE")
    uint32_t instanceNum;
    const OpenRhiPtr(Buffer) instanceBuffer;                    // contains "TopLevelInstance" entries
    uint64_t instanceOffset;
    OpenRhiPtr(Buffer) scratchBuffer;                           // use "GetAccelerationStructureBuildScratchBufferSize" or "GetAccelerationStructureUpdateScratchBufferSize" to determine the required size
    uint64_t scratchOffset;
};

OpenRhiStruct(BuildBottomLevelAccelerationStructureDesc) {
    OpenRhiPtr(AccelerationStructure) dst;
    OpenRhiOptional const OpenRhiPtr(AccelerationStructure) src;    // implies "update" instead of "build" if provided (requires "ALLOW_UPDATE")
    const OpenRhiPtr(BottomLevelGeometryDesc) geometries;
    uint32_t geometryNum;
    OpenRhiPtr(Buffer) scratchBuffer;
    uint64_t scratchOffset;
};

#pragma endregion

//============================================================================================================================================================================================
#pragma region [ Other ]
//============================================================================================================================================================================================

OpenRhiEnum(CopyMode, uint8_t,
    CLONE,
    COMPACT
);

OpenRhiStruct(StridedBufferRegion) {
    const OpenRhiPtr(Buffer) buffer;
    uint64_t offset;
    uint64_t size;
    uint64_t stride;
};

OpenRhiStruct(DispatchRaysDesc) {
    OpenRhi(StridedBufferRegion) raygenShaderRecord;
    OpenRhi(StridedBufferRegion) missShaderBindingTable;
    OpenRhi(StridedBufferRegion) hitShaderBindingTable;
    OpenRhi(StridedBufferRegion) callableShaderBindingTable;

    uint32_t width, height, depth;
};

OpenRhiStruct(DispatchRaysIndirectDesc) {
    uint64_t raygenShaderRecordAddress;
    uint64_t raygenShaderRecordSize;

    uint64_t missShaderBindingTableAddress;
    uint64_t missShaderBindingTableSize;
    uint64_t missShaderBindingTableStride;

    uint64_t hitShaderBindingTableAddress;
    uint64_t hitShaderBindingTableSize;
    uint64_t hitShaderBindingTableStride;

    uint64_t callableShaderBindingTableAddress;
    uint64_t callableShaderBindingTableSize;
    uint64_t callableShaderBindingTableStride;

    uint32_t width, height, depth;
};

#pragma endregion

// Threadsafe: yes
OpenRhiStruct(RayTracingInterface) {
    // Create
    OpenRhi(Result)     (OPENRHI_CALL *CreateRayTracingPipeline)                        (OpenRhiRef(Device) device, const OpenRhiRef(RayTracingPipelineDesc) rayTracingPipelineDesc, OpenRhiOut OpenRhiRef(Pipeline*) pipeline);
    OpenRhi(Result)     (OPENRHI_CALL *CreateAccelerationStructureDescriptor)           (const OpenRhiRef(AccelerationStructure) accelerationStructure, OpenRhiOut OpenRhiRef(Descriptor*) descriptor);

    // Get
    uint64_t        (OPENRHI_CALL *GetAccelerationStructureHandle)                  (const OpenRhiRef(AccelerationStructure) accelerationStructure);
    uint64_t        (OPENRHI_CALL *GetAccelerationStructureUpdateScratchBufferSize) (const OpenRhiRef(AccelerationStructure) accelerationStructure);
    uint64_t        (OPENRHI_CALL *GetAccelerationStructureBuildScratchBufferSize)  (const OpenRhiRef(AccelerationStructure) accelerationStructure);
    uint64_t        (OPENRHI_CALL *GetMicromapBuildScratchBufferSize)               (const OpenRhiRef(Micromap) micromap);

    // For barriers
    OpenRhiPtr(Buffer)  (OPENRHI_CALL *GetAccelerationStructureBuffer)                  (const OpenRhiRef(AccelerationStructure) accelerationStructure);
    OpenRhiPtr(Buffer)  (OPENRHI_CALL *GetMicromapBuffer)                               (const OpenRhiRef(Micromap) micromap);

    // Destroy
    void            (OPENRHI_CALL *DestroyAccelerationStructure)                    (OpenRhiPtr(AccelerationStructure) accelerationStructure);
    void            (OPENRHI_CALL *DestroyMicromap)                                 (OpenRhiPtr(Micromap) micromap);

    // Resources and memory (VK style)
    OpenRhi(Result)     (OPENRHI_CALL *CreateAccelerationStructure)                     (OpenRhiRef(Device) device, const OpenRhiRef(AccelerationStructureDesc) accelerationStructureDesc, OpenRhiOut OpenRhiRef(AccelerationStructure*) accelerationStructure);
    OpenRhi(Result)     (OPENRHI_CALL *CreateMicromap)                                  (OpenRhiRef(Device) device, const OpenRhiRef(MicromapDesc) micromapDesc, OpenRhiOut OpenRhiRef(Micromap*) micromap);
    void            (OPENRHI_CALL *GetAccelerationStructureMemoryDesc)              (const OpenRhiRef(AccelerationStructure) accelerationStructure, OpenRhi(MemoryLocation) memoryLocation, OpenRhiOut OpenRhiRef(MemoryDesc) memoryDesc);
    void            (OPENRHI_CALL *GetMicromapMemoryDesc)                           (const OpenRhiRef(Micromap) micromap, OpenRhi(MemoryLocation) memoryLocation, OpenRhiOut OpenRhiRef(MemoryDesc) memoryDesc);
    OpenRhi(Result)     (OPENRHI_CALL *BindAccelerationStructureMemory)                 (const OpenRhiPtr(BindAccelerationStructureMemoryDesc) bindAccelerationStructureMemoryDescs, uint32_t bindAccelerationStructureMemoryDescNum);
    OpenRhi(Result)     (OPENRHI_CALL *BindMicromapMemory)                              (const OpenRhiPtr(BindMicromapMemoryDesc) bindMicromapMemoryDescs, uint32_t bindMicromapMemoryDescNum);

    // Resources and memory (D3D12 style)
    void            (OPENRHI_CALL *GetAccelerationStructureMemoryDesc2)             (const OpenRhiRef(Device) device, const OpenRhiRef(AccelerationStructureDesc) accelerationStructureDesc, OpenRhi(MemoryLocation) memoryLocation, OpenRhiOut OpenRhiRef(MemoryDesc) memoryDesc); // requires "features.getMemoryDesc2"
    void            (OPENRHI_CALL *GetMicromapMemoryDesc2)                          (const OpenRhiRef(Device) device, const OpenRhiRef(MicromapDesc) micromapDesc, OpenRhi(MemoryLocation) memoryLocation, OpenRhiOut OpenRhiRef(MemoryDesc) memoryDesc); // requires "features.getMemoryDesc2"
    OpenRhi(Result)     (OPENRHI_CALL *CreateCommittedAccelerationStructure)            (OpenRhiRef(Device) device, OpenRhi(MemoryLocation) memoryLocation, float priority, const OpenRhiRef(AccelerationStructureDesc) accelerationStructureDesc, OpenRhiOut OpenRhiRef(AccelerationStructure*) accelerationStructure);
    OpenRhi(Result)     (OPENRHI_CALL *CreateCommittedMicromap)                         (OpenRhiRef(Device) device, OpenRhi(MemoryLocation) memoryLocation, float priority, const OpenRhiRef(MicromapDesc) micromapDesc, OpenRhiOut OpenRhiRef(Micromap*) micromap);
    OpenRhi(Result)     (OPENRHI_CALL *CreatePlacedAccelerationStructure)               (OpenRhiRef(Device) device, OpenRhiOptional OpenRhiPtr(Memory) memory, uint64_t offset, const OpenRhiRef(AccelerationStructureDesc) accelerationStructureDesc, OpenRhiOut OpenRhiRef(AccelerationStructure*) accelerationStructure);
    OpenRhi(Result)     (OPENRHI_CALL *CreatePlacedMicromap)                            (OpenRhiRef(Device) device, OpenRhiOptional OpenRhiPtr(Memory) memory, uint64_t offset, const OpenRhiRef(MicromapDesc) micromapDesc, OpenRhiOut OpenRhiRef(Micromap*) micromap);

    // (HOST) Shader binding table: writes shader group identifiers to "dst" with "dstStride"
    // "dstStride" must be >= "shaderStage.rayTracing.shaderGroupIdentifierSize"; bytes after each identifier are preserved
    // "dst" size must be >= "shaderGroupNum * dstStride" bytes
    OpenRhi(Result)     (OPENRHI_CALL *WriteShaderGroupIdentifiers)                     (const OpenRhiRef(Pipeline) pipeline, uint32_t baseShaderGroupIndex, uint32_t shaderGroupNum, uint32_t dstStride, OpenRhiOut void* dst);

    // Command buffer
    // {
        // Micromap
        void        (OPENRHI_CALL *CmdBuildMicromaps)                               (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiPtr(BuildMicromapDesc) buildMicromapDescs, uint32_t buildMicromapDescNum);
        void        (OPENRHI_CALL *CmdWriteMicromapSizes)                           (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiPtr(Micromap) const* micromaps, uint32_t micromapNum, OpenRhiRef(QueryPool) queryPool, uint32_t queryPoolOffset);
        void        (OPENRHI_CALL *CmdCopyMicromap)                                 (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(Micromap) dst, const OpenRhiRef(Micromap) src, OpenRhi(CopyMode) copyMode);

        // Acceleration structure
        void        (OPENRHI_CALL *CmdBuildTopLevelAccelerationStructures)          (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiPtr(BuildTopLevelAccelerationStructureDesc) buildTopLevelAccelerationStructureDescs, uint32_t buildTopLevelAccelerationStructureDescNum);
        void        (OPENRHI_CALL *CmdBuildBottomLevelAccelerationStructures)       (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiPtr(BuildBottomLevelAccelerationStructureDesc) buildBottomLevelAccelerationStructureDescs, uint32_t buildBottomLevelAccelerationStructureDescNum);
        void        (OPENRHI_CALL *CmdWriteAccelerationStructureSizes)              (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiPtr(AccelerationStructure) const* accelerationStructures, uint32_t accelerationStructureNum, OpenRhiRef(QueryPool) queryPool, uint32_t queryPoolOffset);
        void        (OPENRHI_CALL *CmdCopyAccelerationStructure)                    (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(AccelerationStructure) dst, const OpenRhiRef(AccelerationStructure) src, OpenRhi(CopyMode) copyMode);

        // Ray tracing
        void        (OPENRHI_CALL *CmdDispatchRays)                                 (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(DispatchRaysDesc) dispatchRaysDesc);
        void        (OPENRHI_CALL *CmdDispatchRaysIndirect)                         (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(Buffer) buffer, uint64_t offset); // buffer contains "DispatchRaysIndirectDesc" commands
    // }

    // Native object
    uint64_t        (OPENRHI_CALL* GetAccelerationStructureNativeObject)            (const OpenRhiPtr(AccelerationStructure) accelerationStructure); // ID3D12Resource* or VkAccelerationStructureKHR
    uint64_t        (OPENRHI_CALL* GetMicromapNativeObject)                         (const OpenRhiPtr(Micromap) micromap);                           // ID3D12Resource* or VkMicromapEXT
};

OpenRhiNamespaceEnd
