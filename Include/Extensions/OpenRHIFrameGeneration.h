// OpenRHI AMD frame generation and paced presentation provider
#pragma once

#define OPENRHI_FRAME_GENERATION_H 1
#include "OpenRHISwapChain.h"
#include "OpenRHIUpscaler.h"

OpenRhiNamespaceBegin

OpenRhiForwardStruct(FrameGeneration);

OpenRhiEnum(FrameGenerationTransferFunction, uint8_t,
    SRGB,
    PQ,
    LINEAR
);

OpenRhiStruct(FrameGenerationProvider) {
    uint64_t id;
    char name[128];
};

// Creates a new provider-owned proxy swapchain for this HWND; retire/destroy the previous swapchain first
// D3D12, FSR SDK 2.3.0, FG API 4.0.1 and swapchain API 3.1.7
OpenRhiStruct(FrameGenerationDesc) {
    OpenRhi(SwapChainDesc) swapChain;
    OpenRhi(Dim2_t) maxRenderResolution;
    OpenRhi(UpscalerBits) flags; // HDR, DEPTH_INVERTED, DEPTH_INFINITE, MV_UPSCALED, MV_JITTERED
    OpenRhiOptional uint64_t providerId; // 0 selects the provider's default; otherwise an enumerated id
    bool enableDebugChecking;
    OpenRhi(FrameGenerationTransferFunction) transferFunction;
    float minLuminance; // nits
    float maxLuminance; // nits; 0 selects 80 for SDR/linear and 1000 for PQ
};

// Configure before preparation for the same monotonically increasing frameId
OpenRhiStruct(FrameGenerationConfigureDesc) {
    uint64_t frameId;
    bool enabled;
    OpenRhiOptional OpenRhiPtr(Texture) hudlessColor; // COMMON/GENERAL/ALL through Present+Wait, display size and swapchain format
    OpenRhiOptional OpenRhiPtr(Texture) ui; // RGBA8_UNORM or RGBA16_SFLOAT, premultiplied or straight RGBA; COMMON/GENERAL/ALL through Present+Wait, display size
    bool uiPremultiplied;
};

OpenRhiStruct(FrameGenerationPrepareDesc) {
    uint64_t frameId;
    OpenRhi(Dim2_t) renderResolution; // active top-left guide region; allocated depth/render-resolution motion may extend up to maxRenderResolution
    OpenRhiPtr(Texture) depth; // R32_SFLOAT hardware depth values, COMMON/GENERAL/ALL before/after preparation
    OpenRhiPtr(Texture) motionVectors; // RG16_SFLOAT or RG32_SFLOAT motion, COMMON/GENERAL/ALL before/after preparation
    OpenRhi(Float2_t) cameraJitter; // pixel jitter
    OpenRhi(Float2_t) motionVectorScale; // converts stored motion to pixels
    float frameTimeMs;
    float cameraNear; // physical near plane; provider converts inverted/infinite depth like UpscalerInterface
    float cameraFar; // physical far plane; ignored with DEPTH_INFINITE
    float verticalFov; // radians
    float viewSpaceToMetersFactor;
    float cameraPosition[3];
    float cameraUp[3];
    float cameraRight[3];
    float cameraForward[3];
    bool reset;
};

// Recorded/generated counters describe SDK dispatch, not scanout; DXGI count is separately provider-reported
OpenRhiStruct(FrameGenerationStats) {
    uint64_t preparedFrames;
    uint64_t generatedDispatches;
    uint64_t generatedFramesRecorded;
    uint64_t presentCalls;
    uint64_t lastFrameId;
    uint32_t dxgiPresentCount;
    OpenRhi(Result) lastDispatchResult;
    OpenRhi(Result) lastPresentResult;
    OpenRhi(FrameGenerationProvider) provider;
    bool configuredEnabled;
};

// Externally synchronize each context; caller owns barriers, command recording, submission and scene/UI resources
// Prepare outside rendering; submit the game queue before PresentFrameGeneration
// SDK records interpolation during Present and owns its presentation/pacing threads and queues
// WaitFrameGeneration drains provider presents; also retire caller queue work before reusing resources or destruction
// Stop/deconfigure generation before destruction; keep the device/queue/HWND and borrowed resources alive throughout
OpenRhiStruct(FrameGenerationInterface) {
    OpenRhi(Result) (OPENRHI_CALL *EnumerateFrameGenerationProviders) (const OpenRhiRef(Device) device, OpenRhiOptional OpenRhiPtr(FrameGenerationProvider) providers, NonOpenRhiRef(uint32_t) providerNum);
    OpenRhi(Result) (OPENRHI_CALL *CreateFrameGeneration) (OpenRhiRef(Device) device, const OpenRhiRef(FrameGenerationDesc) desc, OpenRhiOut OpenRhiRef(FrameGeneration*) context);
    void (OPENRHI_CALL *DestroyFrameGeneration) (OpenRhiPtr(FrameGeneration) context);
    OpenRhiPtr(Texture) const* (OPENRHI_CALL *GetFrameGenerationTextures) (const OpenRhiRef(FrameGeneration) context, NonOpenRhiRef(uint32_t) textureNum);
    OpenRhi(Result) (OPENRHI_CALL *AcquireFrameGenerationTexture) (OpenRhiRef(FrameGeneration) context, NonOpenRhiRef(uint32_t) textureIndex);
    OpenRhi(Result) (OPENRHI_CALL *ConfigureFrameGeneration) (OpenRhiRef(FrameGeneration) context, const OpenRhiRef(FrameGenerationConfigureDesc) desc);
    OpenRhi(Result) (OPENRHI_CALL *CmdPrepareFrameGeneration) (OpenRhiRef(CommandBuffer) commandBuffer, OpenRhiRef(FrameGeneration) context, const OpenRhiRef(FrameGenerationPrepareDesc) desc);
    OpenRhi(Result) (OPENRHI_CALL *PresentFrameGeneration) (OpenRhiRef(FrameGeneration) context, uint8_t syncInterval); // 0 allows tearing when supported; 1 synchronizes presentation
    OpenRhi(Result) (OPENRHI_CALL *WaitFrameGeneration) (OpenRhiRef(FrameGeneration) context);
    void (OPENRHI_CALL *GetFrameGenerationStats) (const OpenRhiRef(FrameGeneration) context, OpenRhiOut OpenRhiRef(FrameGenerationStats) stats);
    void* (OPENRHI_CALL *GetFrameGenerationSwapChainNativeObject) (OpenRhiRef(FrameGeneration) context); // borrowed IDXGISwapChain4 for AntiLagInterface attachment
};

OpenRhiNamespaceEnd
