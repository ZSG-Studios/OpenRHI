// © 2026 NVIDIA Corporation

// Goal: wrapping native Metal objects into OpenRHI objects

#pragma once

#define OPENRHI_WRAPPER_METAL_H 1

#include "OpenRHIDeviceCreation.h"

OpenRhiNamespaceBegin

// Native pointers use these exact metal-cpp types: MTL::Device*, MTL4::CommandQueue*, MTL::Buffer*, MTL::Texture*, and MTL::SharedEvent*.
// OpenRHI retains every supplied native object until the corresponding OpenRHI object is destroyed

// A collection of queues of the same type
OpenRhiStruct(QueueFamilyMetalDesc) {
    OpenRhiOptional void* const* mtl4Queues;                // MTL4::CommandQueue*, if not provided, will be created
    uint32_t queueNum;
    OpenRhi(QueueType) queueType;
};

OpenRhiStruct(DeviceCreationMetalDesc) {
    OpenRhiOptional const OpenRhiPtr(DeviceRequirements) requirements;
    OpenRhiOptional OpenRhiPtr(DeviceCompatibilityReport) compatibilityReport;
    OpenRhiOptional OpenRhi(ProfilePolicy) profilePolicy;
    void* mtlDevice;                                    // MTL::Device*
    const OpenRhiPtr(QueueFamilyMetalDesc) queueFamilies;
    uint32_t queueFamilyNum;
    OpenRhiOptional OpenRhi(CallbackInterface) callbackInterface;
    OpenRhiOptional OpenRhi(AllocationCallbacks) allocationCallbacks;

    // Switches (disabled by default)
    bool enableOpenRHIValidation;
    bool enableMemoryZeroInitialization;                // Metal memory is always zero-initialized
};

OpenRhiStruct(BufferMetalDesc) {
    void* mtlBuffer;                                    // MTL::Buffer*
    OpenRhi(BufferDesc) desc;                               // complete OpenRHI metadata for the native buffer
};

OpenRhiStruct(TextureMetalDesc) {
    void* mtlTexture;                                   // MTL::Texture*
    OpenRhi(TextureDesc) desc;                              // complete OpenRHI metadata for the native texture
};

OpenRhiStruct(FenceMetalDesc) {
    void* mtlSharedEvent;                               // MTL::SharedEvent*
};

OpenRhiStruct(WrapperMetalInterface) {
    OpenRhi(Result) (OPENRHI_CALL *CreateBufferMetal)       (OpenRhiRef(Device) device, const OpenRhiRef(BufferMetalDesc) bufferMetalDesc, OpenRhiOut OpenRhiRef(Buffer*) buffer);
    OpenRhi(Result) (OPENRHI_CALL *CreateTextureMetal)      (OpenRhiRef(Device) device, const OpenRhiRef(TextureMetalDesc) textureMetalDesc, OpenRhiOut OpenRhiRef(Texture*) texture);
    OpenRhi(Result) (OPENRHI_CALL *CreateFenceMetal)        (OpenRhiRef(Device) device, const OpenRhiRef(FenceMetalDesc) fenceMetalDesc, OpenRhiOut OpenRhiRef(Fence*) fence);

};

#if defined(__APPLE__)
OPENRHI_API OpenRhi(Result) OPENRHI_CALL openrhiCreateDeviceFromMetalDevice(const OpenRhiRef(DeviceCreationMetalDesc) deviceDesc, OpenRhiOut OpenRhiRef(Device*) device);
#endif

OpenRhiNamespaceEnd
