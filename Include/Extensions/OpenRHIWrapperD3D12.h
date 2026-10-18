// © 2021 NVIDIA Corporation

// Goal: wrapping native D3D12 objects into OpenRHI objects

#pragma once

#define OPENRHI_WRAPPER_D3D12_H 1

#include "OpenRHIDeviceCreation.h"
#include "OpenRHIRayTracing.h"

typedef int32_t DXGIFormat;

NonOpenRhiForwardStruct(AGSContext);
NonOpenRhiForwardStruct(ID3D12Heap);
NonOpenRhiForwardStruct(ID3D12Fence);
NonOpenRhiForwardStruct(ID3D12Device);
NonOpenRhiForwardStruct(ID3D12Resource);
NonOpenRhiForwardStruct(ID3D12CommandList);
NonOpenRhiForwardStruct(ID3D12CommandQueue);
NonOpenRhiForwardStruct(ID3D12DescriptorHeap);
NonOpenRhiForwardStruct(ID3D12CommandAllocator);

OpenRhiNamespaceBegin

// A collection of queues of the same type
OpenRhiStruct(QueueFamilyD3D12Desc) {
    OpenRhiOptional ID3D12CommandQueue* const* d3d12Queues; // if not provided, will be created
    uint32_t queueNum;
    OpenRhi(QueueType) queueType;
};

OpenRhiStruct(DeviceCreationD3D12Desc) {
    OpenRhiOptional const OpenRhiPtr(DeviceRequirements) requirements;
    OpenRhiOptional OpenRhiPtr(DeviceCompatibilityReport) compatibilityReport;
    OpenRhiOptional OpenRhi(ProfilePolicy) profilePolicy;
    ID3D12Device* d3d12Device;
    const OpenRhiPtr(QueueFamilyD3D12Desc) queueFamilies;
    uint32_t queueFamilyNum;
    OpenRhiOptional AGSContext* agsContext;
    OpenRhiOptional OpenRhi(CallbackInterface) callbackInterface;
    OpenRhiOptional OpenRhi(AllocationCallbacks) allocationCallbacks;
    OpenRhiOptional uint32_t d3dShaderExtRegister;  // vendor specific shader extensions (default is "OPENRHI_SHADER_EXT_REGISTER", space is always "0")
    OpenRhiOptional uint32_t d3dZeroBufferSize;     // no "memset" functionality in D3D, "CmdZeroBuffer" implemented via a bunch of copies (4 Mb by default)

    // Switches (disabled by default)
    bool enableOpenRHIValidation;
    bool enableMemoryZeroInitialization;        // page-clears are fast, not enabled by default to match VK (the extension needed)

    // Switches (enabled by default)
    bool disableD3D12EnhancedBarriers;          // even if AgilitySDK is in use, some apps still use legacy barriers. It can be important for integrations
    bool disableNVAPIInitialization;            // at least NVAPI requires calling "NvAPI_Initialize" in DLL/EXE where the device is created
};

OpenRhiStruct(CommandBufferD3D12Desc) {
    ID3D12CommandList* d3d12CommandList;
    OpenRhiOptional ID3D12CommandAllocator* d3d12CommandAllocator; // needed only for "BeginCommandBuffer"
};

OpenRhiStruct(DescriptorPoolD3D12Desc) {
    ID3D12DescriptorHeap* d3d12ResourceDescriptorHeap;
    ID3D12DescriptorHeap* d3d12SamplerDescriptorHeap;

    // Allocation limits (D3D12 unrelated, but must match expected usage)
    uint32_t descriptorSetMaxNum;
};

OpenRhiStruct(BufferD3D12Desc) {
    ID3D12Resource* d3d12Resource;
    OpenRhiOptional const OpenRhiPtr(BufferDesc) desc;  // not all information can be retrieved from the resource if not provided
    OpenRhiOptional uint32_t structureStride;       // must be provided if used as a structured or raw buffer
};

OpenRhiStruct(TextureD3D12Desc) {
    ID3D12Resource* d3d12Resource;
    OpenRhiOptional DXGIFormat format;              // must be provided "as a compatible typed format" if the resource is typeless
};

OpenRhiStruct(MemoryD3D12Desc) {
    ID3D12Heap* d3d12Heap;
    uint64_t offset;
};

OpenRhiStruct(FenceD3D12Desc) {
    ID3D12Fence* d3d12Fence;
};

OpenRhiStruct(AccelerationStructureD3D12Desc) {
    ID3D12Resource* d3d12Resource;
    OpenRhi(AccelerationStructureBits) flags;

    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO
    uint64_t size;
    uint64_t buildScratchSize;
    uint64_t updateScratchSize;
};

// Threadsafe: yes
OpenRhiStruct(WrapperD3D12Interface) {
    OpenRhi(Result) (OPENRHI_CALL *CreateCommandBufferD3D12)            (OpenRhiRef(Device) device, const OpenRhiRef(CommandBufferD3D12Desc) commandBufferD3D12Desc, OpenRhiOut OpenRhiRef(CommandBuffer*) commandBuffer);
    OpenRhi(Result) (OPENRHI_CALL *CreateDescriptorPoolD3D12)           (OpenRhiRef(Device) device, const OpenRhiRef(DescriptorPoolD3D12Desc) descriptorPoolD3D12Desc, OpenRhiOut OpenRhiRef(DescriptorPool*) descriptorPool);
    OpenRhi(Result) (OPENRHI_CALL *CreateBufferD3D12)                   (OpenRhiRef(Device) device, const OpenRhiRef(BufferD3D12Desc) bufferD3D12Desc, OpenRhiOut OpenRhiRef(Buffer*) buffer);
    OpenRhi(Result) (OPENRHI_CALL *CreateTextureD3D12)                  (OpenRhiRef(Device) device, const OpenRhiRef(TextureD3D12Desc) textureD3D12Desc, OpenRhiOut OpenRhiRef(Texture*) texture);
    OpenRhi(Result) (OPENRHI_CALL *CreateMemoryD3D12)                   (OpenRhiRef(Device) device, const OpenRhiRef(MemoryD3D12Desc) memoryD3D12Desc, OpenRhiOut OpenRhiRef(Memory*) memory);
    OpenRhi(Result) (OPENRHI_CALL *CreateFenceD3D12)                    (OpenRhiRef(Device) device, const OpenRhiRef(FenceD3D12Desc) fenceD3D12Desc, OpenRhiOut OpenRhiRef(Fence*) fence);
    OpenRhi(Result) (OPENRHI_CALL *CreateAccelerationStructureD3D12)    (OpenRhiRef(Device) device, const OpenRhiRef(AccelerationStructureD3D12Desc) accelerationStructureD3D12Desc, OpenRhiOut OpenRhiRef(AccelerationStructure*) accelerationStructure);
    // TODO-VIDEO: add CreateVideoSession
};

#if defined(_WIN32)
OPENRHI_API OpenRhi(Result) OPENRHI_CALL openrhiCreateDeviceFromD3D12Device(const OpenRhiRef(DeviceCreationD3D12Desc) deviceDesc, OpenRhiOut OpenRhiRef(Device*) device);
#endif

OpenRhiNamespaceEnd
