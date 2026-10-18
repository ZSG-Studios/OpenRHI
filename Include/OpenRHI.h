// © 2021 NVIDIA Corporation

/*
Overview:
 - Generalized common denominator for VK, D3D12 and native Metal
    - VK spec: https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html
       - Best practices: https://developer.nvidia.com/blog/vulkan-dos-donts/
       - Feature support coverage: https://vulkan.gpuinfo.org/
    - D3D12 spec: https://microsoft.github.io/DirectX-Specs/
       - Feature support coverage: https://d3d12infodb.boolka.dev/

Goals:
 - a low-overhead RHI with explicit native graphics API control
 - generalization and unification of D3D12 and VK
 - explicitness (providing access to low-level features of modern GAPIs)
 - quality-of-life and high-level extensions (e.g., streaming and upscaling)
 - low overhead
 - cross-platform and platform independence (AMD/INTEL friendly)

Non-goals:
 - exposing entities not existing in GAPIs
 - hidden management of any kind (except for some high-level extensions where it's desired)
 - automatic barriers (better handled in a higher-level abstraction)

Thread safety:
 - Threadsafe: yes - free-threaded access
 - Threadsafe: no  - external synchronization required, i.e. one thread at a time (additional restrictions can apply)
 - Threadsafe: ?   - unclear status

Implicit:
 - Create*         - thread safe
 - Destroy*        - not thread safe (because of VK)
 - Cmd*            - not thread safe
*/

#pragma once

#define OPENRHI_VERSION 1
#define OPENRHI_VERSION_DATE "18 October 2026"

// C/C++ compatible interface (auto-selection or via "OPENRHI_FORCE_C" macro)
#include "OpenRHIDescs.h"

// Can be used with "name", "openrhi::name" and "OpenRhiName"
#define OPENRHI_INTERFACE(name) #name, sizeof(name)

OpenRhiNamespaceBegin

// Example: Result result = openrhiGetInterface(device, OPENRHI_INTERFACE(CoreInterface), &coreInterface)
OPENRHI_API OpenRhi(Result) OPENRHI_CALL openrhiGetInterface(const OpenRhiRef(Device) device, const char* interfaceName, size_t interfaceSize, void* interfacePtr);

// Annotations for profiling tools: host
// - Host annotations currently use NVTX (NVIDIA Nsight Systems)
// - Device (command buffer and queue) annotations use GAPI or PIX (if "WinPixEventRuntime.dll" is nearby)
// - Colorization requires PIX or NVTX
OPENRHI_API void OPENRHI_CALL openrhiBeginAnnotation(const char* name, uint32_t bgra);  // start a named range
OPENRHI_API void OPENRHI_CALL openrhiEndAnnotation();                                   // end the last opened range
OPENRHI_API void OPENRHI_CALL openrhiAnnotation(const char* name, uint32_t bgra);       // emit a named simultaneous event
OPENRHI_API void OPENRHI_CALL openrhiSetThreadName(const char* name);                   // assign a name to the current thread

// Threadsafe: yes
OpenRhiStruct(CoreInterface) {
    // Get
    const OpenRhiRef(DeviceDesc)    (OPENRHI_CALL *GetDeviceDesc)           (const OpenRhiRef(Device) device);
    const OpenRhiRef(BufferDesc)    (OPENRHI_CALL *GetBufferDesc)           (const OpenRhiRef(Buffer) buffer);
    const OpenRhiRef(TextureDesc)   (OPENRHI_CALL *GetTextureDesc)          (const OpenRhiRef(Texture) texture);
    OpenRhi(FormatSupportBits)      (OPENRHI_CALL *GetFormatSupport)        (const OpenRhiRef(Device) device, OpenRhi(Format) format);

    // Returns one of the pre-created queues (see "DeviceCreationDesc" or wrapper extensions)
    // Return codes: "UNSUPPORTED" (no queues of "queueType") or "INVALID_ARGUMENT" (if "queueIndex" is out of bounds).
    // Getting "COMPUTE" and/or "COPY" queues switches VK sharing mode to "VK_SHARING_MODE_CONCURRENT" for resources created without "queueExclusive" flag.
    // This approach is used to minimize number of "queue ownership transfers", but also adds a requirement to "get" all async queues BEFORE creation of
    // resources participating into multi-queue activities. Explicit use of "queueExclusive" removes any restrictions.
    OpenRhi(Result)         (OPENRHI_CALL *GetQueue)                        (OpenRhiRef(Device) device, OpenRhi(QueueType) queueType, uint32_t queueIndex, OpenRhiOut OpenRhiRef(Queue*) queue);

    // Create (doesn't assume allocation of big chunks of memory on the device, but it happens for some entities implicitly)
    OpenRhi(Result)         (OPENRHI_CALL *CreateCommandAllocator)          (OpenRhiRef(Queue) queue, OpenRhiOut OpenRhiRef(CommandAllocator*) commandAllocator);
    OpenRhi(Result)         (OPENRHI_CALL *CreateCommandBuffer)             (OpenRhiRef(CommandAllocator) commandAllocator, OpenRhiOut OpenRhiRef(CommandBuffer*) commandBuffer);
    OpenRhi(Result)         (OPENRHI_CALL *CreateFence)                     (OpenRhiRef(Device) device, uint64_t initialValue, OpenRhiOut OpenRhiRef(Fence*) fence);
    OpenRhi(Result)         (OPENRHI_CALL *CreateDescriptorPool)            (OpenRhiRef(Device) device, const OpenRhiRef(DescriptorPoolDesc) descriptorPoolDesc, OpenRhiOut OpenRhiRef(DescriptorPool*) descriptorPool);
    OpenRhi(Result)         (OPENRHI_CALL *CreatePipelineLayout)            (OpenRhiRef(Device) device, const OpenRhiRef(PipelineLayoutDesc) pipelineLayoutDesc, OpenRhiOut OpenRhiRef(PipelineLayout*) pipelineLayout);
    OpenRhi(Result)         (OPENRHI_CALL *CreateGraphicsPipeline)          (OpenRhiRef(Device) device, const OpenRhiRef(GraphicsPipelineDesc) graphicsPipelineDesc, OpenRhiOut OpenRhiRef(Pipeline*) pipeline);
    OpenRhi(Result)         (OPENRHI_CALL *CreateComputePipeline)           (OpenRhiRef(Device) device, const OpenRhiRef(ComputePipelineDesc) computePipelineDesc, OpenRhiOut OpenRhiRef(Pipeline*) pipeline);
    OpenRhi(Result)         (OPENRHI_CALL *CreatePipelineCache)             (OpenRhiRef(Device) device, const OpenRhiRef(PipelineCacheDesc) pipelineCacheDesc, OpenRhiOut OpenRhiRef(PipelineCache*) pipelineCache); // "OUT_OF_DATE" is returned on stale data, try to start over with an empty cache
    OpenRhi(Result)         (OPENRHI_CALL *CreateQueryPool)                 (OpenRhiRef(Device) device, const OpenRhiRef(QueryPoolDesc) queryPoolDesc, OpenRhiOut OpenRhiRef(QueryPool*) queryPool);
    OpenRhi(Result)         (OPENRHI_CALL *CreateSampler)                   (OpenRhiRef(Device) device, const OpenRhiRef(SamplerDesc) samplerDesc, OpenRhiOut OpenRhiRef(Descriptor*) sampler);
    OpenRhi(Result)         (OPENRHI_CALL *CreateBufferView)                (const OpenRhiRef(BufferViewDesc) bufferViewDesc, OpenRhiOut OpenRhiRef(Descriptor*) bufferView);
    OpenRhi(Result)         (OPENRHI_CALL *CreateTextureView)               (const OpenRhiRef(TextureViewDesc) textureViewDesc, OpenRhiOut OpenRhiRef(Descriptor*) textureView);

    // Destroy
    void                (OPENRHI_CALL *DestroyCommandAllocator)         (OpenRhiPtr(CommandAllocator) commandAllocator);
    void                (OPENRHI_CALL *DestroyCommandBuffer)            (OpenRhiPtr(CommandBuffer) commandBuffer);
    void                (OPENRHI_CALL *DestroyDescriptorPool)           (OpenRhiPtr(DescriptorPool) descriptorPool);
    void                (OPENRHI_CALL *DestroyBuffer)                   (OpenRhiPtr(Buffer) buffer);
    void                (OPENRHI_CALL *DestroyTexture)                  (OpenRhiPtr(Texture) texture);
    void                (OPENRHI_CALL *DestroyDescriptor)               (OpenRhiPtr(Descriptor) descriptor);
    void                (OPENRHI_CALL *DestroyPipelineLayout)           (OpenRhiPtr(PipelineLayout) pipelineLayout);
    void                (OPENRHI_CALL *DestroyPipeline)                 (OpenRhiPtr(Pipeline) pipeline);
    void                (OPENRHI_CALL *DestroyPipelineCache)            (OpenRhiPtr(PipelineCache) pipelineCache);
    void                (OPENRHI_CALL *DestroyQueryPool)                (OpenRhiPtr(QueryPool) queryPool);
    void                (OPENRHI_CALL *DestroyFence)                    (OpenRhiPtr(Fence) fence);

    // Memory
    OpenRhi(Result)         (OPENRHI_CALL *AllocateMemory)                  (OpenRhiRef(Device) device, const OpenRhiRef(AllocateMemoryDesc) allocateMemoryDesc, OpenRhiOut OpenRhiRef(Memory*) memory);
    void                (OPENRHI_CALL *FreeMemory)                      (OpenRhiPtr(Memory) memory);

    // Resources and memory (VK style)
    // - create a resource (buffer or texture)
    // - use "Get[Resource]MemoryDesc" to get "MemoryDesc" ("usageBits" and "MemoryLocation" affect returned "MemoryType")
    // - (optional) group returned "MemoryDesc"s by "MemoryType", but don't group if "mustBeDedicated = true"
    // - (optional) sort returned "MemoryDesc"s by alignment
    // - call "AllocateMemory" (even if "mustBeDedicated = true")
    // - call "Bind[Resource]Memory" to bind resources to "Memory" objects
    // - (optional) "CalculateAllocationNumber" and "AllocateAndBindMemory" from "HelperInterface" interface simplify this process for buffers and textures
    OpenRhi(Result)         (OPENRHI_CALL *CreateBuffer)                    (OpenRhiRef(Device) device, const OpenRhiRef(BufferDesc) bufferDesc, OpenRhiOut OpenRhiRef(Buffer*) buffer);
    OpenRhi(Result)         (OPENRHI_CALL *CreateTexture)                   (OpenRhiRef(Device) device, const OpenRhiRef(TextureDesc) textureDesc, OpenRhiOut OpenRhiRef(Texture*) texture);
    void                (OPENRHI_CALL *GetBufferMemoryDesc)             (const OpenRhiRef(Buffer) buffer, OpenRhi(MemoryLocation) memoryLocation, OpenRhiOut OpenRhiRef(MemoryDesc) memoryDesc);
    void                (OPENRHI_CALL *GetTextureMemoryDesc)            (const OpenRhiRef(Texture) texture, OpenRhi(MemoryLocation) memoryLocation, OpenRhiOut OpenRhiRef(MemoryDesc) memoryDesc);
    OpenRhi(Result)         (OPENRHI_CALL *BindBufferMemory)                (const OpenRhiPtr(BindBufferMemoryDesc) bindBufferMemoryDescs, uint32_t bindBufferMemoryDescNum);
    OpenRhi(Result)         (OPENRHI_CALL *BindTextureMemory)               (const OpenRhiPtr(BindTextureMemoryDesc) bindTextureMemoryDescs, uint32_t bindTextureMemoryDescNum);

    // Resources and memory (D3D12 style)
    // - "Get[Resource]MemoryDesc2" requires "maintenance4" support on Vulkan
    // - "memory, offset" pair can be replaced with a "OpenRhi[Device/DeviceUpload/HostUpload/HostReadback]Heap" macro to create a placed resource in the corresponding memory using VMA (AMD Virtual Memory Allocator) implicitly
    void                (OPENRHI_CALL *GetBufferMemoryDesc2)            (const OpenRhiRef(Device) device, const OpenRhiRef(BufferDesc) bufferDesc, OpenRhi(MemoryLocation) memoryLocation, OpenRhiOut OpenRhiRef(MemoryDesc) memoryDesc); // requires "features.getMemoryDesc2"
    void                (OPENRHI_CALL *GetTextureMemoryDesc2)           (const OpenRhiRef(Device) device, const OpenRhiRef(TextureDesc) textureDesc, OpenRhi(MemoryLocation) memoryLocation, OpenRhiOut OpenRhiRef(MemoryDesc) memoryDesc); // requires "features.getMemoryDesc2"
    OpenRhi(Result)         (OPENRHI_CALL *CreateCommittedBuffer)           (OpenRhiRef(Device) device, OpenRhi(MemoryLocation) memoryLocation, float priority, const OpenRhiRef(BufferDesc) bufferDesc, OpenRhiOut OpenRhiRef(Buffer*) buffer);
    OpenRhi(Result)         (OPENRHI_CALL *CreateCommittedTexture)          (OpenRhiRef(Device) device, OpenRhi(MemoryLocation) memoryLocation, float priority, const OpenRhiRef(TextureDesc) textureDesc, OpenRhiOut OpenRhiRef(Texture*) texture);
    OpenRhi(Result)         (OPENRHI_CALL *CreatePlacedBuffer)              (OpenRhiRef(Device) device, OpenRhiPtr(Memory) memory, uint64_t offset, const OpenRhiRef(BufferDesc) bufferDesc, OpenRhiOut OpenRhiRef(Buffer*) buffer);
    OpenRhi(Result)         (OPENRHI_CALL *CreatePlacedTexture)             (OpenRhiRef(Device) device, OpenRhiPtr(Memory) memory, uint64_t offset, const OpenRhiRef(TextureDesc) textureDesc, OpenRhiOut OpenRhiRef(Texture*) texture);

    // Descriptor set management (entities don't require destroying)
    // - if "ALLOW_UPDATE_AFTER_SET" not used, descriptor sets (and data pointed to by descriptors) must be updated before "CmdSetDescriptorSet"
    // - "ResetDescriptorPool" resets the entire pool and wipes out all allocated descriptor sets. "DescriptorSet" is a tiny struct (<= 48 bytes),
    //   so lots of descriptor sets can be created in advance and reused without calling "ResetDescriptorPool"
    // - when directly indexed, a descriptor pool backs the directly indexed arrays:
    //   - D3D12: "GetDescriptorSetOffsets" returns offsets in resource and sampler descriptor heaps
    //     - these offsets are needed in shaders, if the corresponding descriptor set is not the first allocated from the descriptor pool
    //   - VK: "GetDescriptorSetOffsets" returns "0"
    //     - match resource and sampler heap bindings to the compiled Slang reflection
    OpenRhi(Result)         (OPENRHI_CALL *AllocateDescriptorSets)          (OpenRhiRef(DescriptorPool) descriptorPool, const OpenRhiRef(PipelineLayout) pipelineLayout, uint32_t setIndex, OpenRhiOut OpenRhiPtr(DescriptorSet)* descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum);
    void                (OPENRHI_CALL *UpdateDescriptorRanges)          (const OpenRhiPtr(UpdateDescriptorRangeDesc) updateDescriptorRangeDescs, uint32_t updateDescriptorRangeDescNum);
    void                (OPENRHI_CALL *CopyDescriptorRanges)            (const OpenRhiPtr(CopyDescriptorRangeDesc) copyDescriptorRangeDescs, uint32_t copyDescriptorRangeDescNum);
    void                (OPENRHI_CALL *ResetDescriptorPool)             (OpenRhiRef(DescriptorPool) descriptorPool);
    void                (OPENRHI_CALL *GetDescriptorSetOffsets)         (const OpenRhiRef(DescriptorSet) descriptorSet, OpenRhiOut NonOpenRhiRef(uint32_t) resourceHeapOffset, OpenRhiOut NonOpenRhiRef(uint32_t) samplerHeapOffset);

    // Command buffer (one time submit)
    OpenRhi(Result)         (OPENRHI_CALL *BeginCommandBuffer)              (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiPtr(DescriptorPool) descriptorPool);
    // {                {
        // Set descriptor pool (initially can be set via "BeginCommandBuffer")
        void                (OPENRHI_CALL *CmdSetDescriptorPool)        (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(DescriptorPool) descriptorPool);

        // Resource binding (expect "CmdSetPipelineLayout" to be called first)
        void                (OPENRHI_CALL *CmdSetPipelineLayout)        (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhi(BindPoint) bindPoint, const OpenRhiRef(PipelineLayout) pipelineLayout);
        void                (OPENRHI_CALL *CmdSetDescriptorSet)         (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(SetDescriptorSetDesc) setDescriptorSetDesc);
        void                (OPENRHI_CALL *CmdSetRootConstants)         (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(SetRootConstantsDesc) setRootConstantsDesc);
        void                (OPENRHI_CALL *CmdSetRootDescriptor)        (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(SetRootDescriptorDesc) setRootDescriptorDesc);

        // Pipeline
        void                (OPENRHI_CALL *CmdSetPipeline)              (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(Pipeline) pipeline);

        // Barrier (outside of rendering)
        void                (OPENRHI_CALL *CmdBarrier)                  (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(BarrierDesc) barrierDesc);

        // Input assembly
        void                (OPENRHI_CALL *CmdSetIndexBuffer)           (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(Buffer) buffer, uint64_t offset, OpenRhi(IndexType) indexType);
        void                (OPENRHI_CALL *CmdSetVertexBuffers)         (OpenRhiRef(CommandBuffer) commandBuffer, uint32_t baseSlot, const OpenRhiPtr(VertexBufferDesc) vertexBufferDescs, uint32_t vertexBufferNum);

        // Initial state (mandatory)
        void                (OPENRHI_CALL *CmdSetViewports)             (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiPtr(Viewport) viewports, uint32_t viewportNum);
        void                (OPENRHI_CALL *CmdSetScissors)              (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiPtr(Rect) rects, uint32_t rectNum);

        // Initial state (if enabled)
        void                (OPENRHI_CALL *CmdSetStencilReference)      (OpenRhiRef(CommandBuffer) commandBuffer, uint8_t frontRef, uint8_t backRef); // "backRef" requires "features.independentFrontAndBackStencilReferenceAndMasks"
        void                (OPENRHI_CALL *CmdSetDepthBounds)           (OpenRhiRef(CommandBuffer) commandBuffer, float boundsMin, float boundsMax); // requires "features.depthBoundsTest"
        void                (OPENRHI_CALL *CmdSetBlendConstants)        (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(Color32f) color);
        void                (OPENRHI_CALL *CmdSetSampleLocations)       (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiPtr(SampleLocation) locations, OpenRhi(Sample_t) locationNum, OpenRhi(Sample_t) sampleNum); // requires "tiers.sampleLocations != 0"
        void                (OPENRHI_CALL *CmdSetShadingRate)           (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(ShadingRateDesc) shadingRateDesc); // requires "tiers.shadingRate != 0"
        void                (OPENRHI_CALL *CmdSetDepthBias)             (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(DepthBiasDesc) depthBiasDesc); // requires "features.dynamicDepthBias", actually it's an override

        // Graphics
        void                (OPENRHI_CALL *CmdBeginRendering)           (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(RenderingDesc) renderingDesc);
        // {                {
            // Clear ("rects" require the corresponding "features.rectColorClears" or "features.rectDepthStencilClears")
            void                (OPENRHI_CALL *CmdClearAttachments)     (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiPtr(ClearAttachmentDesc) clearAttachmentDescs, uint32_t clearAttachmentDescNum, const OpenRhiPtr(Rect) rects, uint32_t rectNum);

            // Draw
            void                (OPENRHI_CALL *CmdDraw)                 (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(DrawDesc) drawDesc);
            void                (OPENRHI_CALL *CmdDrawIndexed)          (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(DrawIndexedDesc) drawIndexedDesc);

            // Draw indirect:
            // - drawNum = min(drawNum, countBuffer ? countBuffer[countBufferOffset] : INF)
            // - see "Modified draw command signatures"
            void                (OPENRHI_CALL *CmdDrawIndirect)         (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(Buffer) buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, OpenRhiOptional const OpenRhiPtr(Buffer) countBuffer, uint64_t countBufferOffset); // "buffer" contains "Draw(Base)Desc" commands
            void                (OPENRHI_CALL *CmdDrawIndexedIndirect)  (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(Buffer) buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, OpenRhiOptional const OpenRhiPtr(Buffer) countBuffer, uint64_t countBufferOffset); // "buffer" contains "DrawIndexed(Base)Desc" commands
        // }                }
        void                (OPENRHI_CALL *CmdEndRendering)             (OpenRhiRef(CommandBuffer) commandBuffer);

        // Compute (outside of rendering)
        void                (OPENRHI_CALL *CmdDispatch)                 (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(DispatchDesc) dispatchDesc);
        void                (OPENRHI_CALL *CmdDispatchIndirect)         (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(Buffer) buffer, uint64_t offset); // buffer contains "DispatchDesc" commands

        // Copy (outside of rendering)
        void                (OPENRHI_CALL *CmdCopyBuffer)               (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(Buffer) dstBuffer, uint64_t dstOffset, const OpenRhiRef(Buffer) srcBuffer, uint64_t srcOffset, uint64_t size);
        void                (OPENRHI_CALL *CmdCopyTexture)              (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(Texture) dstTexture, OpenRhiOptional const OpenRhiPtr(TextureRegionDesc) dstRegion, const OpenRhiRef(Texture) srcTexture, OpenRhiOptional const OpenRhiPtr(TextureRegionDesc) srcRegion); // METAL: a single-plane depth-stencil copy requires identical non-multisampled formats
        void                (OPENRHI_CALL *CmdUploadBufferToTexture)    (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(Texture) dstTexture, const OpenRhiRef(TextureRegionDesc) dstRegion, const OpenRhiRef(Buffer) srcBuffer, const OpenRhiRef(TextureDataLayoutDesc) srcDataLayout);
        void                (OPENRHI_CALL *CmdReadbackTextureToBuffer)  (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(Buffer) dstBuffer, const OpenRhiRef(TextureDataLayoutDesc) dstDataLayout, const OpenRhiRef(Texture) srcTexture, const OpenRhiRef(TextureRegionDesc) srcRegion);
        void                (OPENRHI_CALL *CmdZeroBuffer)               (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(Buffer) buffer, uint64_t offset, uint64_t size);

        // Resolve (outside of rendering). METAL: color only, use "RenderingDesc::depth.resolveDst" for depth
        void                (OPENRHI_CALL *CmdResolveTexture)           (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(Texture) dstTexture, OpenRhiOptional const OpenRhiPtr(TextureRegionDesc) dstRegion, const OpenRhiRef(Texture) srcTexture, OpenRhiOptional const OpenRhiPtr(TextureRegionDesc) srcRegion, OpenRhi(ResolveOp) resolveOp); // "features.regionResolve" is needed for region specification

        // Clear (outside of rendering)
        void                (OPENRHI_CALL *CmdClearStorage)             (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(ClearStorageDesc) clearStorageDesc);

        // Query and timestamps (outside of rendering, except Begin/End query)
        void                (OPENRHI_CALL *CmdResetQueries)             (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(QueryPool) queryPool, uint32_t offset, uint32_t num); // can't be used with a COPY queue
        void                (OPENRHI_CALL *CmdBeginQuery)               (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(QueryPool) queryPool, uint32_t offset);
        void                (OPENRHI_CALL *CmdEndQuery)                 (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(QueryPool) queryPool, uint32_t offset); // writes a timestamp for timestamp query pools
        void                (OPENRHI_CALL *CmdCopyQueries)              (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(QueryPool) queryPool, uint32_t offset, uint32_t num, OpenRhiRef(Buffer) dstBuffer, uint64_t dstOffset);

        // Annotations for profiling tools: command buffer
        void                (OPENRHI_CALL *CmdBeginAnnotation)          (OpenRhiRef(CommandBuffer) commandBuffer, const char* name, uint32_t bgra);
        void                (OPENRHI_CALL *CmdEndAnnotation)            (OpenRhiRef(CommandBuffer) commandBuffer);
        void                (OPENRHI_CALL *CmdAnnotation)               (OpenRhiRef(CommandBuffer) commandBuffer, const char* name, uint32_t bgra);
    // }                }
    OpenRhi(Result)         (OPENRHI_CALL *EndCommandBuffer)                (OpenRhiRef(CommandBuffer) commandBuffer);

    // Annotations for profiling tools
    void                (OPENRHI_CALL *QueueBeginAnnotation)            (OpenRhiRef(Queue) queue, const char* name, uint32_t bgra);
    void                (OPENRHI_CALL *QueueEndAnnotation)              (OpenRhiRef(Queue) queue);
    void                (OPENRHI_CALL *QueueAnnotation)                 (OpenRhiRef(Queue) queue, const char* name, uint32_t bgra);

    // Queries and timestamps
    void                (OPENRHI_CALL *ResetQueries)                    (OpenRhiRef(QueryPool) queryPool, uint32_t offset, uint32_t num); // on host
    uint32_t            (OPENRHI_CALL *GetQuerySize)                    (const OpenRhiRef(QueryPool) queryPool);
    void                (OPENRHI_CALL *GetCalibratedTimestamps)         (OpenRhiRef(Queue) queue, NonOpenRhiRef(uint64_t) timestampGPU, NonOpenRhiRef(uint64_t) timestampCPU);

    // Work submission and synchronization
    OpenRhi(Result)         (OPENRHI_CALL *QueueSubmit)                     (OpenRhiRef(Queue) queue, const OpenRhiRef(QueueSubmitDesc) queueSubmitDesc); // to device
    OpenRhi(Result)         (OPENRHI_CALL *QueueWaitIdle)                   (OpenRhiPtr(Queue) queue);
    OpenRhi(Result)         (OPENRHI_CALL *DeviceWaitIdle)                  (OpenRhiPtr(Device) device);
    void                (OPENRHI_CALL *Wait)                            (OpenRhiRef(Fence) fence, uint64_t value); // on host
    uint64_t            (OPENRHI_CALL *GetFenceValue)                   (OpenRhiRef(Fence) fence);

    // Command allocator
    void                (OPENRHI_CALL *ResetCommandAllocator)           (OpenRhiRef(CommandAllocator) commandAllocator);

    // Host address
    // D3D12: persistent mapping, "Map/Unmap" do nothing
    // VK: persistent mapping, but "Unmap" can do a flush if underlying memory is not "HOST_COHERENT" (unlikely)
    void*               (OPENRHI_CALL *MapBuffer)                       (OpenRhiRef(Buffer) buffer, uint64_t offset, uint64_t size);
    void                (OPENRHI_CALL *UnmapBuffer)                     (OpenRhiRef(Buffer) buffer);

    // Synchronous host copies
    // - all prior GPU access to the copied subresources must be complete before the call
    // - host memory is no longer accessed and readback data is available after the call returns
    // - textures must be created with "TextureUsageBits::HOST_TRANSFER"
    // - destination regions in a single "UploadHostMemoryToTexture" call must not overlap
    OpenRhi(Result)         (OPENRHI_CALL *UploadHostMemoryToTexture)       (OpenRhiRef(Queue) queue, const OpenRhiPtr(UploadHostMemoryToTextureDesc) copyDescs, uint32_t copyDescNum);
    OpenRhi(Result)         (OPENRHI_CALL *ReadbackTextureToHostMemory)     (OpenRhiRef(Queue) queue, const OpenRhiPtr(ReadbackTextureToHostMemoryDesc) copyDescs, uint32_t copyDescNum);

    // Device address (aka GPU virtual address or "0" if unsupported)
    uint64_t            (OPENRHI_CALL *GetBufferDeviceAddress)          (const OpenRhiRef(Buffer) buffer);

    // Pipeline cache (PSO blob storage, persisted across runs)
    // - Threadsafe: no, external synchronization required, call after all pipeline creations using this cache have completed
    // - 2-call pattern: pass "dst = NULL" to query required "size", then call again with allocated "dst"
    OpenRhi(Result)         (OPENRHI_CALL *GetPipelineCacheData)            (OpenRhiRef(PipelineCache) pipelineCache, OpenRhiOut void* dst, NonOpenRhiRef(uint64_t) size);

    // Debug name for any object declared as "OpenRhiForwardStruct" (skipped for buffers & textures in D3D if they are not bound to a memory)
    void                (OPENRHI_CALL *SetDebugName)                    (OpenRhiPtr(Object) object, const char* name);

    // Native objects                                                                                            _D3D12 (latest interface)____|_VK_________________________________
    void*               (OPENRHI_CALL *GetDeviceNativeObject)           (const OpenRhiPtr(Device) device);               // ID3D12Device*               | VkDevice
    void*               (OPENRHI_CALL *GetQueueNativeObject)            (const OpenRhiPtr(Queue) queue);                 // ID3D12CommandQueue*         | VkQueue
    void*               (OPENRHI_CALL *GetCommandBufferNativeObject)    (const OpenRhiPtr(CommandBuffer) commandBuffer); // ID3D12CommandList*          | VkCommandBuffer
    uint64_t            (OPENRHI_CALL *GetBufferNativeObject)           (const OpenRhiPtr(Buffer) buffer);               // ID3D12Resource*             | VkBuffer
    uint64_t            (OPENRHI_CALL *GetTextureNativeObject)          (const OpenRhiPtr(Texture) texture);             // ID3D12Resource*             | VkImage
    uint64_t            (OPENRHI_CALL *GetDescriptorNativeObject)       (const OpenRhiPtr(Descriptor) descriptor);       // D3D12_CPU_DESCRIPTOR_HANDLE | VkImageView/VkBufferView/VkSampler
};

OpenRhiNamespaceEnd
