// © 2021 NVIDIA Corporation

// Goal: device creation

#pragma once

#define OPENRHI_DEVICE_CREATION_H 1

OpenRhiNamespaceBegin

OpenRhiEnum(Message, uint8_t,
    INFO,
    WARNING,
    ERROR // "wingdi.h" must not be included after
);

OpenRhiEnum(DeviceLostInfoLevel, uint8_t,
    NONE,
    BASIC,   // backend-supported basic device lost reporting
    VERBOSE  // D3D12: additionally enables DRED breadcrumb contexts
);

OpenRhiStruct(DeviceLostDump) {
    OpenRhiOptional const void* data; // opaque backend-specific binary data, OpenRHI-owned and valid until device destruction
    uint64_t size;                // bytes, 0 if "data" is NULL
};

// Callbacks must be thread safe
OpenRhiStruct(AllocationCallbacks) {
    void* (OPENRHI_CALL *Allocate)(void* userArg, size_t size, size_t alignment);
    void* (OPENRHI_CALL *Reallocate)(void* userArg, void* memory, size_t size, size_t alignment);
    void (OPENRHI_CALL *Free)(void* userArg, void* memory);
    void* userArg;
    bool disable3rdPartyAllocationCallbacks; // to use "AllocationCallbacks" only for OpenRHI needs
};

OpenRhiStruct(CallbackInterface) {
    void (OPENRHI_CALL *MessageCallback)(OpenRhi(Message) messageType, const char* file, uint32_t line, const char* message, void* userArg);
    OpenRhiOptional void (OPENRHI_CALL *AbortExecution)(void* userArg); // break on "Message::ERROR" if provided
    OpenRhiOptional void* userArg;
};

// Use largest offset for the resource type planned to be used as an unbounded array
OpenRhiStruct(VKBindingOffsets) {
    uint32_t sRegister; // samplers
    uint32_t tRegister; // shader resources, including acceleration structures (SRVs)
    uint32_t bRegister; // constant buffers
    uint32_t uRegister; // storage shader resources (UAVs)
};

OpenRhiStruct(VKExtensions) {
    const char* const* instanceExtensions;
    uint32_t instanceExtensionNum;
    const char* const* deviceExtensions;
    uint32_t deviceExtensionNum;
};

// A collection of queues of the same type
OpenRhiStruct(QueueFamilyDesc) {
    OpenRhiOptional const float* queuePriorities;   // [-1; 1]: low < 0, normal = 0, high > 0 ("queueNum" entries expected)
    uint32_t queueNum;
    OpenRhi(QueueType) queueType;
};

// D3D12: process-global settings affect all subsequently created D3D12 devices; later OpenRHI device creation does not disable them
OpenRhiStruct(DeviceCreationDesc) {
    OpenRhi(GraphicsAPI) graphicsAPI;
    OpenRhiOptional const OpenRhiPtr(DeviceRequirements) requirements;
    OpenRhiOptional OpenRhiPtr(DeviceCompatibilityReport) compatibilityReport;
    OpenRhiOptional OpenRhi(ProfilePolicy) profilePolicy;
    OpenRhiOptional OpenRhi(Robustness) robustness;
    OpenRhiOptional OpenRhi(DeviceLostInfoLevel) deviceLostInfoLevel; // D3D12: process-global
    OpenRhiOptional const OpenRhiPtr(AdapterDesc) adapterDesc;
    OpenRhiOptional OpenRhi(CallbackInterface) callbackInterface;
    OpenRhiOptional OpenRhi(AllocationCallbacks) allocationCallbacks;

    // One "GRAPHICS" queue is created by default
    OpenRhiOptional const OpenRhiPtr(QueueFamilyDesc) queueFamilies;
    OpenRhiOptional uint32_t queueFamilyNum;        // put "GRAPHICS" queue at the beginning of the list

    // D3D specific
    OpenRhiOptional uint32_t d3dShaderExtRegister;  // vendor specific shader extensions (default is "OPENRHI_SHADER_EXT_REGISTER", space is always "0")
    OpenRhiOptional uint32_t d3dZeroBufferSize;     // no "memset" functionality in D3D, "CmdZeroBuffer" implemented via a bunch of copies (4 Mb by default)

    // Vulkan specific
    OpenRhi(VKBindingOffsets) vkBindingOffsets;
    OpenRhiOptional OpenRhi(VKExtensions) vkExtensions; // to enable

    // Switches (disabled by default)
    bool enableOpenRHIValidation;                   // embedded validation layer, checks for OpenRHI specifics
    bool enableGraphicsAPIValidation;           // GAPI-provided validation layer (D3D12: process-global; METAL: ignored, set "MTL_DEBUG_LAYER=1" instead)
    bool enableD3D12RayTracingValidation;       // slow but useful, can only be enabled if envvar "NV_ALLOW_RAYTRACING_VALIDATION" is set to "1"
    bool enableMemoryZeroInitialization;        // page-clears are fast, but memory is not cleared by default in VK

    // Switches (enabled by default)
    bool disableVKRayTracing;                   // to save CPU memory in some implementations
    bool disableD3D12EnhancedBarriers;          // even if AgilitySDK is in use, some apps still use legacy barriers. It can be important for integrations
};

// if "adapterDescs == NULL", then "adapterDescNum" is set to the number of adapters
// else "adapterDescNum" must be set to number of elements in "adapterDescs"
OPENRHI_API OpenRhi(Result) OPENRHI_CALL openrhiEnumerateAdapters(OpenRhiPtr(AdapterDesc) adapterDescs, NonOpenRhiRef(uint32_t) adapterDescNum);

OPENRHI_API OpenRhi(Result) OPENRHI_CALL openrhiCreateDevice(const OpenRhiRef(DeviceCreationDesc) deviceCreationDesc, OpenRhiOut OpenRhiRef(Device*) device);
OPENRHI_API void OPENRHI_CALL openrhiDestroyDevice(OpenRhiPtr(Device) device);

// It's global state for D3D, not needed for VK because validation is tied to the logical device
OPENRHI_API void OPENRHI_CALL openrhiReportLiveObjects();

// Reports backend-specific device lost diagnostics and returns a binary dump (if available), "device" must be in a lost state (see "DEVICE_LOST")
OPENRHI_API OpenRhi(Result) OPENRHI_CALL openrhiReportDeviceLostInfo(OpenRhiRef(Device) device, OpenRhiOut OpenRhiRef(DeviceLostDump) deviceLostDump);

OPENRHI_API OpenRhi(DeviceRequirements) OPENRHI_CALL openrhiGetPlatformRequirements(OpenRhi(GraphicsAPI) graphicsAPI);

// Probe creates and destroys a temporary device; call explicitly outside frame execution
OPENRHI_API OpenRhi(Result) OPENRHI_CALL openrhiQueryAdapterSupport(const OpenRhiRef(DeviceCreationDesc) desc, OpenRhiRef(DeviceCompatibilityReport) report);
// Returned profile is immutable and remains valid until device destruction
OPENRHI_API const OpenRhiPtr(DeviceProfileDesc) OPENRHI_CALL openrhiGetDeviceProfile(const OpenRhiRef(Device) device);
OPENRHI_API OpenRhi(Result) OPENRHI_CALL openrhiEvaluateDeviceSupport(const OpenRhiRef(DeviceDesc) desc, const OpenRhiRef(DeviceRequirements) requirements, OpenRhiRef(DeviceCompatibilityReport) report);

OpenRhiNamespaceEnd
