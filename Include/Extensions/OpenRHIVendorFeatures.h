// OpenRHI native vendor feature adapters
#pragma once

#define OPENRHI_VENDOR_FEATURES_H 1

OpenRhiNamespaceBegin

OpenRhiForwardStruct(FidelityFXContext);
OpenRhiForwardStruct(NeuralRadianceCache);
OpenRhiForwardStruct(AntiLagContext);

// Native descriptions use the pinned SDK headers documented in VENDOR_FEATURES.md
// Native resources come from CoreInterface::Get*NativeObject and remain caller-owned
// Calls on a context require external synchronization; finish GPU work before destroying it
OpenRhiEnum(FidelityFXEffect, uint8_t,
    RAY_REGENERATION,
    RADIANCE_CACHE
);

OpenRhiStruct(FidelityFXDesc) {
    OpenRhi(FidelityFXEffect) effect;
    const void* nativeDesc; // optional: null creates a provider-query session; otherwise ffxCreateContextDescDenoiser or ffxCreateContextDescRadianceCache; backend supplied by OpenRHI
};

// Windows D3D12, FidelityFX SDK 2.3.0; descriptions and chained objects must live until destruction
// Query accepts a provider-query session for global queries; nativeDesc is an ffxQueryDescHeader chain
// Dispatch accepts the full signal/debug chain and supplies commandList in its copied root description
// Caller supplies resource states, barriers, shader data, submission, fences and native state rebinding
OpenRhiStruct(FidelityFXInterface) {
    OpenRhi(Result) (OPENRHI_CALL *CreateFidelityFXContext)(OpenRhiRef(Device) device, const OpenRhiRef(FidelityFXDesc) desc, OpenRhiOut OpenRhiRef(FidelityFXContext*) context);
    void (OPENRHI_CALL *DestroyFidelityFXContext)(OpenRhiPtr(FidelityFXContext) context);
    OpenRhi(Result) (OPENRHI_CALL *ConfigureFidelityFX)(OpenRhiRef(FidelityFXContext) context, const void* nativeDesc);
    OpenRhi(Result) (OPENRHI_CALL *QueryFidelityFX)(OpenRhiRef(FidelityFXContext) context, void* nativeDesc);
    OpenRhi(Result) (OPENRHI_CALL *CmdDispatchFidelityFX)(OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(FidelityFXContext) context, const void* nativeDesc);
};

OpenRhiStruct(NeuralRadianceCacheDesc) {
    const void* globalSettings; // nrc::GlobalSettings; one identical settings set per native API across live contexts
    const void* contextSettings; // nrc::ContextSettings
    const void* buffers; // optional nrc::d3d12::Buffers for caller-managed allocation
    const char* networkConfigFilename; // optional, keep live until the next Configure or destruction
};

// NRC 0.16 native ABI, Windows D3D12; Configure performs preparation outside frame execution
// Use native shader helpers from the selected NRC SDK in Slang query/update producers
OpenRhiStruct(NeuralRadianceCacheInterface) {
    OpenRhi(Result) (OPENRHI_CALL *CreateNeuralRadianceCache)(OpenRhiRef(Device) device, const OpenRhiRef(NeuralRadianceCacheDesc) desc, OpenRhiOut OpenRhiRef(NeuralRadianceCache*) cache);
    void (OPENRHI_CALL *DestroyNeuralRadianceCache)(OpenRhiPtr(NeuralRadianceCache) cache);
    OpenRhi(Result) (OPENRHI_CALL *ConfigureNeuralRadianceCache)(OpenRhiRef(NeuralRadianceCache) cache, const void* contextSettings, const void* buffers, const char* networkConfigFilename);
    OpenRhi(Result) (OPENRHI_CALL *GetNeuralRadianceCacheAllocationInfo)(const OpenRhiRef(Device) device, const OpenRhiRef(NeuralRadianceCacheDesc) desc, void* allocationInfo); // nrc::BuffersAllocationInfo
    OpenRhi(Result) (OPENRHI_CALL *GetNeuralRadianceCacheInitialBufferState)(const OpenRhiRef(NeuralRadianceCache) cache, const void* allocationInfo, OpenRhiOut NonOpenRhiRef(uint32_t) nativeState); // nrc::AllocationInfo, D3D12_RESOURCE_STATES
    const void* (OPENRHI_CALL *GetNeuralRadianceCacheBuffers)(const OpenRhiRef(NeuralRadianceCache) cache); // borrowed native Buffers, valid until Configure/destruction
    OpenRhi(Result) (OPENRHI_CALL *GetNeuralRadianceCacheShaderConstants)(const OpenRhiRef(NeuralRadianceCache) cache, void* constants); // NrcConstants
    OpenRhi(Result) (OPENRHI_CALL *CmdBeginNeuralRadianceCacheFrame)(OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(NeuralRadianceCache) cache, const void* frameSettings); // nrc::FrameSettings
    OpenRhi(Result) (OPENRHI_CALL *CmdQueryAndTrainNeuralRadianceCache)(OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(NeuralRadianceCache) cache, float* trainingLoss);
    OpenRhi(Result) (OPENRHI_CALL *CmdResolveNeuralRadianceCache)(OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(NeuralRadianceCache) cache, uint64_t nativeOutput); // ID3D12Resource*
    OpenRhi(Result) (OPENRHI_CALL *EndNeuralRadianceCacheFrame)(OpenRhiRef(Queue) queue, OpenRhiRef(NeuralRadianceCache) cache); // after submission on the same queue
};

// Anti-Lag 2 D3D12 public API, pinned at 390aa4a8c8655d0ae6e90079db2c85e103a96da3
// Update once immediately before polling input; mark rendering after submission, set frame type before Present
// A native frame-generation swap chain may use AttachAntiLagSwapChain to receive the SDK context through private data
OpenRhiStruct(AntiLagInterface) {
    OpenRhi(Result) (OPENRHI_CALL *CreateAntiLagContext)(OpenRhiRef(Device) device, OpenRhiOut OpenRhiRef(AntiLagContext*) context);
    void (OPENRHI_CALL *DestroyAntiLagContext)(OpenRhiPtr(AntiLagContext) context);
    OpenRhi(Result) (OPENRHI_CALL *UpdateAntiLag)(OpenRhiRef(AntiLagContext) context, bool enabled, uint32_t maxFPS);
    OpenRhi(Result) (OPENRHI_CALL *MarkAntiLagEndOfRendering)(OpenRhiRef(AntiLagContext) context);
    OpenRhi(Result) (OPENRHI_CALL *SetAntiLagFrameType)(OpenRhiRef(AntiLagContext) context, bool interpolated);
    OpenRhi(Result) (OPENRHI_CALL *AttachAntiLagSwapChain)(OpenRhiRef(AntiLagContext) context, void* nativeSwapChain); // IDXGISwapChain*, null detaches
};

OpenRhiNamespaceEnd
