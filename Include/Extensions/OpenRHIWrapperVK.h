// © 2021 NVIDIA Corporation

// Goal: wrapping native VK objects into OpenRHI objects

#pragma once

#define OPENRHI_WRAPPER_VK_H 1

#include "OpenRHIDeviceCreation.h"
#include "OpenRHIRayTracing.h"

typedef void* VKHandle;
typedef int32_t VKEnum;
typedef uint32_t VKFlags;
typedef uint64_t VKNonDispatchableHandle;

OpenRhiNamespaceBegin

// A collection of queues of the same type
OpenRhiStruct(QueueFamilyVKDesc) {
    uint32_t queueNum;
    OpenRhi(QueueType) queueType;
    uint32_t familyIndex;
};

OpenRhiStruct(DeviceCreationVKDesc) {
    OpenRhiOptional const OpenRhiPtr(DeviceRequirements) requirements;
    OpenRhiOptional OpenRhiPtr(DeviceCompatibilityReport) compatibilityReport;
    OpenRhiOptional OpenRhi(ProfilePolicy) profilePolicy;
    OpenRhiOptional OpenRhi(CallbackInterface) callbackInterface;
    OpenRhiOptional OpenRhi(AllocationCallbacks) allocationCallbacks;
    OpenRhiOptional const char* libraryPath;
    OpenRhi(VKBindingOffsets) vkBindingOffsets;
    OpenRhi(VKExtensions) vkExtensions;                     // enabled
    VKHandle vkInstance;
    VKHandle vkDevice;
    OpenRhi(DeviceCapabilityBits) enabledCapabilities; // enabled on the imported logical device; intersected with physical capabilities
    VKHandle vkPhysicalDevice;
    const OpenRhiPtr(QueueFamilyVKDesc) queueFamilies;
    uint32_t queueFamilyNum;
    uint8_t minorVersion;                               // >= 3

    // Switches (disabled by default)
    bool enableOpenRHIValidation;
    bool enableMemoryZeroInitialization;                // page-clears are fast, but memory is not cleared by default in VK
};

OpenRhiStruct(CommandAllocatorVKDesc) {
    VKNonDispatchableHandle vkCommandPool;
    OpenRhi(QueueType) queueType;
};

OpenRhiStruct(CommandBufferVKDesc) {
    VKHandle vkCommandBuffer;
    OpenRhi(QueueType) queueType;
};

OpenRhiStruct(DescriptorPoolVKDesc) {
    VKNonDispatchableHandle vkDescriptorPool;
    uint32_t descriptorSetMaxNum;
};

OpenRhiStruct(BufferVKDesc) {
    VKNonDispatchableHandle vkBuffer;
    uint64_t size;
    OpenRhiOptional uint32_t structureStride;               // must be provided if used as a structured or raw buffer
    OpenRhiOptional uint8_t* mappedMemory;                  // must be provided if the underlying memory is mapped
    OpenRhiOptional VKNonDispatchableHandle vkDeviceMemory; // must be provided *only* if the mapped memory exists and *not* HOST_COHERENT
    OpenRhiOptional uint64_t deviceAddress;                 // must be provided for ray tracing
};

OpenRhiStruct(TextureVKDesc) {
    VKNonDispatchableHandle vkImage;
    VKEnum vkFormat;
    VKEnum vkImageType;
    VKFlags vkImageUsageFlags;
    OpenRhi(Dim_t) width;
    OpenRhi(Dim_t) height;
    OpenRhi(Dim_t) depth;
    OpenRhi(Dim_t) mipNum;
    OpenRhi(Dim_t) layerNum;
    OpenRhi(Sample_t) sampleNum;
};

OpenRhiStruct(MemoryVKDesc) {
    VKNonDispatchableHandle vkDeviceMemory;
    OpenRhiOptional uint64_t offset;
    OpenRhiOptional void* mappedMemory; // at "offset"
    uint64_t size;
    uint32_t memoryTypeIndex;
};

OpenRhiStruct(PipelineVKDesc) {
    VKNonDispatchableHandle vkPipeline;
    VKEnum vkPipelineBindPoint;
};

OpenRhiStruct(QueryPoolVKDesc) {
    VKNonDispatchableHandle vkQueryPool;
    VKEnum vkQueryType;
};

OpenRhiStruct(FenceVKDesc) {
    VKNonDispatchableHandle vkTimelineSemaphore;
};

OpenRhiStruct(AccelerationStructureVKDesc) {
    VKNonDispatchableHandle vkAccelerationStructure;
    VKNonDispatchableHandle vkBuffer;
    uint64_t bufferSize;
    uint64_t buildScratchSize;
    uint64_t updateScratchSize;
    OpenRhi(AccelerationStructureBits) flags;
};

// Threadsafe: yes
OpenRhiStruct(WrapperVKInterface) {
    OpenRhi(Result) (OPENRHI_CALL *CreateCommandAllocatorVK)        (OpenRhiRef(Device) device, const OpenRhiRef(CommandAllocatorVKDesc) commandAllocatorVKDesc, OpenRhiOut OpenRhiRef(CommandAllocator*) commandAllocator);
    OpenRhi(Result) (OPENRHI_CALL *CreateCommandBufferVK)           (OpenRhiRef(Device) device, const OpenRhiRef(CommandBufferVKDesc) commandBufferVKDesc, OpenRhiOut OpenRhiRef(CommandBuffer*) commandBuffer);
    OpenRhi(Result) (OPENRHI_CALL *CreateDescriptorPoolVK)          (OpenRhiRef(Device) device, const OpenRhiRef(DescriptorPoolVKDesc) descriptorPoolVKDesc, OpenRhiOut OpenRhiRef(DescriptorPool*) descriptorPool);
    OpenRhi(Result) (OPENRHI_CALL *CreateBufferVK)                  (OpenRhiRef(Device) device, const OpenRhiRef(BufferVKDesc) bufferVKDesc, OpenRhiOut OpenRhiRef(Buffer*) buffer);
    OpenRhi(Result) (OPENRHI_CALL *CreateTextureVK)                 (OpenRhiRef(Device) device, const OpenRhiRef(TextureVKDesc) textureVKDesc, OpenRhiOut OpenRhiRef(Texture*) texture);
    OpenRhi(Result) (OPENRHI_CALL *CreateMemoryVK)                  (OpenRhiRef(Device) device, const OpenRhiRef(MemoryVKDesc) memoryVKDesc, OpenRhiOut OpenRhiRef(Memory*) memory);
    OpenRhi(Result) (OPENRHI_CALL *CreatePipelineVK)                (OpenRhiRef(Device) device, const OpenRhiRef(PipelineVKDesc) pipelineVKDesc, OpenRhiOut OpenRhiRef(Pipeline*) pipeline);
    OpenRhi(Result) (OPENRHI_CALL *CreateQueryPoolVK)               (OpenRhiRef(Device) device, const OpenRhiRef(QueryPoolVKDesc) queryPoolVKDesc, OpenRhiOut OpenRhiRef(QueryPool*) queryPool);
    OpenRhi(Result) (OPENRHI_CALL *CreateFenceVK)                   (OpenRhiRef(Device) device, const OpenRhiRef(FenceVKDesc) fenceVKDesc, OpenRhiOut OpenRhiRef(Fence*) fence);
    OpenRhi(Result) (OPENRHI_CALL *CreateAccelerationStructureVK)   (OpenRhiRef(Device) device, const OpenRhiRef(AccelerationStructureVKDesc) accelerationStructureVKDesc, OpenRhiOut OpenRhiRef(AccelerationStructure*) accelerationStructure);
    // TODO-VIDEO: add CreateVideoSession

    uint32_t    (OPENRHI_CALL *GetQueueFamilyIndexVK)           (const OpenRhiRef(Queue) queue);
    VKHandle    (OPENRHI_CALL *GetPhysicalDeviceVK)             (const OpenRhiRef(Device) device);
    VKHandle    (OPENRHI_CALL *GetInstanceVK)                   (const OpenRhiRef(Device) device);
    void*       (OPENRHI_CALL *GetInstanceProcAddrVK)           (const OpenRhiRef(Device) device);
    void*       (OPENRHI_CALL *GetDeviceProcAddrVK)             (const OpenRhiRef(Device) device);
};

#if defined(__linux__) || defined(__ANDROID__)
OPENRHI_API OpenRhi(Result) OPENRHI_CALL openrhiCreateDeviceFromVKDevice(const OpenRhiRef(DeviceCreationVKDesc) deviceDesc, OpenRhiOut OpenRhiRef(Device*) device);
#endif

OpenRhiNamespaceEnd
