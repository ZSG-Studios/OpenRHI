// © 2026 NVIDIA Corporation

// Goal: true bindless resource access through directly indexed descriptor heaps
// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html
// https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_descriptor_heap.html

#pragma once

#define OPENRHI_DESCRIPTOR_HEAP_H 1

OpenRhiNamespaceBegin

OpenRhiForwardStruct(DescriptorHeap);       // layout-free directly indexed descriptor heap

// At least one capacity must be non-zero
OpenRhiStruct(DescriptorHeapDesc) {
    uint32_t resourceDescriptorNum;     // 0 - no resource heap
    uint32_t samplerDescriptorNum;      // 0 - no sampler heap
};

OpenRhiStruct(WriteResourceDescriptorsDesc) {
    const OpenRhiPtr(Descriptor) resource;  // any descriptor except "MUTABLE", "INPUT_ATTACHMENT" and "SAMPLER"
    uint32_t descriptorIndex;
};

OpenRhiStruct(WriteSamplerDescriptorsDesc) {
    const OpenRhiPtr(Descriptor) sampler;   // must be a SAMPLER
    uint32_t descriptorIndex;
};

// Threadsafe: yes, except writes to overlapping descriptor indices require external synchronization
OpenRhiStruct(DescriptorHeapInterface) {
    // Create
    OpenRhi(Result)     (OPENRHI_CALL *CreateDescriptorHeap)            (OpenRhiRef(Device) device, const OpenRhiRef(DescriptorHeapDesc) descriptorHeapDesc, OpenRhiOut OpenRhiRef(DescriptorHeap*) descriptorHeap);

    // Destroy
    void            (OPENRHI_CALL *DestroyDescriptorHeap)           (OpenRhiPtr(DescriptorHeap) descriptorHeap);

    // Updates (on HOST)
    // GPU-side updates are not supported because D3D12 has no equivalent functionality
    OpenRhi(Result)     (OPENRHI_CALL *WriteResourceDescriptors)        (OpenRhiRef(DescriptorHeap) descriptorHeap, const OpenRhiPtr(WriteResourceDescriptorsDesc) writeDescs, uint32_t writeDescNum);
    OpenRhi(Result)     (OPENRHI_CALL *WriteSamplerDescriptors)         (OpenRhiRef(DescriptorHeap) descriptorHeap, const OpenRhiPtr(WriteSamplerDescriptorsDesc) writeDescs, uint32_t writeDescNum);

    // Command buffer
    // {
        void            (OPENRHI_CALL *CmdSetDescriptorHeap)        (OpenRhiRef(CommandBuffer) commandBuffer, const OpenRhiRef(DescriptorHeap) descriptorHeap);
    // }
};

OpenRhiNamespaceEnd
