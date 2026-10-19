// © 2024 NVIDIA Corporation

// Goal: minimizing latency between input sampling and frame presentation

#pragma once

#define OPENRHI_LOW_LATENCY_H 1

OpenRhiNamespaceBegin

OpenRhiForwardStruct(SwapChain);
OpenRhiForwardStruct(Queue);

// us = microseconds

OpenRhiEnum(LatencyMarker, uint8_t,     // Should be called:
    SIMULATION_START    = 0,            // at the start of the simulation execution each frame, but after the call to "LatencySleep"
    SIMULATION_END      = 1,            // at the end of the simulation execution each frame
    RENDER_SUBMIT_START = 2,            // at the beginning of the render submission execution each frame (must not span into asynchronous rendering)
    RENDER_SUBMIT_END   = 3,            // at the end of the render submission execution each frame
    INPUT_SAMPLE        = 6,            // immediately before polling input, between simulation start/end
    TRIGGER_FLASH       = 7,            // click-triggered latency analyzer flash
    PC_LATENCY_PING     = 8,            // Windows Reflex statistics ping (D3D12)
    OUT_OF_BAND_RENDER_SUBMIT_START = 9,
    OUT_OF_BAND_RENDER_SUBMIT_END = 10,
    OUT_OF_BAND_PRESENT_START = 11,
    OUT_OF_BAND_PRESENT_END = 12
);

OpenRhiStruct(LatencySleepMode) {
    uint32_t minIntervalUs;             // minimum allowed frame interval (0 - no frame rate limit)
    bool lowLatencyMode;                // low latency mode enablement
    bool lowLatencyBoost;               // hint to increase performance to provide additional latency savings at a cost of increased power consumption
    bool useMinQueueTime;               // D3D12: optimize submission overlaps
};

OpenRhiEnum(LatencyQueueRole, uint8_t,
    RENDER,
    PRESENT,
    IGNORED,                   // D3D12
    RENDER_PRESENT,           // D3D12
    EXPLICIT_COPY             // D3D12
);

OpenRhiStruct(LatencySleepStatus) { // D3D12 native sleep/driver status
    bool lowLatencyMode;
    bool fullscreenVRR;
    bool driverVsync;
    bool gameSleep;
    bool fullscreenIndependentFlip;
    bool dynamicFrameGeneration;
    uint8_t frameGenerationMultiplier;
    uint32_t sleepIntervalUs;
    uint32_t dynamicFrameTimeTargetUs;
};

OpenRhiStruct(LatencyReport) {          // The time stamp written:
    uint64_t inputSampleTimeUs;         // when "INPUT_SAMPLE" marker is set
    uint64_t simulationStartTimeUs;     // when "SIMULATION_START" marker is set
    uint64_t simulationEndTimeUs;       // when "SIMULATION_END" marker is set
    uint64_t renderSubmitStartTimeUs;   // when "RENDER_SUBMIT_START" marker is set
    uint64_t renderSubmitEndTimeUs;     // when "RENDER_SUBMIT_END" marker is set
    uint64_t presentStartTimeUs;        // right before "Present"
    uint64_t presentEndTimeUs;          // right after "Present"
    uint64_t driverStartTimeUs;         // when the first "QueueSubmitTrackable" is called
    uint64_t driverEndTimeUs;           // when the final "QueueSubmitTrackable" hands off from the driver
    uint64_t osRenderQueueStartTimeUs;
    uint64_t osRenderQueueEndTimeUs;
    uint64_t gpuRenderStartTimeUs;      // when the first submission reaches the GPU
    uint64_t gpuRenderEndTimeUs;        // when the final submission finishes on the GPU
    uint64_t presentId;
    uint64_t cameraConstructedTimeUs;   // D3D12
    uint32_t gpuActiveRenderTimeUs;     // D3D12
    uint32_t gpuFrameTimeUs;            // D3D12
    uint32_t crossAdapterCopyTimeUs;    // D3D12
    uint32_t frameGenerationTimeUs;     // D3D12
};

// Multi-swapchain is supported only by VK
// "QueueSubmitDesc::swapChain" must be used to associate work submissions with a low latency swap chain
// "presentId" must be non-zero and match the value passed to "QueueSubmit" and "QueuePresent" for the frame
// Threadsafe: yes
OpenRhiStruct(LowLatencyInterface) {
    OpenRhi(Result)     (OPENRHI_CALL   *SetLatencySleepMode)   (OpenRhiRef(SwapChain) swapChain, const OpenRhiRef(LatencySleepMode) latencySleepMode);
    OpenRhi(Result)     (OPENRHI_CALL   *SetLatencyMarker)      (OpenRhiRef(SwapChain) swapChain, uint64_t presentId, OpenRhi(LatencyMarker) latencyMarker);
    OpenRhi(Result)     (OPENRHI_CALL   *LatencySleep)          (OpenRhiRef(SwapChain) swapChain, uint64_t presentId); // call once before "INPUT_SAMPLE"
    OpenRhi(Result)     (OPENRHI_CALL   *GetLatencyReport)      (const OpenRhiRef(SwapChain) swapChain, OpenRhiOut OpenRhiRef(LatencyReport) latencyReport);
    OpenRhi(Result)     (OPENRHI_CALL   *GetLatencySleepStatus) (const OpenRhiRef(SwapChain) swapChain, OpenRhiOut OpenRhiRef(LatencySleepStatus) status);
    OpenRhi(Result)     (OPENRHI_CALL   *SetLatencyQueueRole)   (OpenRhiRef(Queue) queue, OpenRhi(LatencyQueueRole) role);
};

OpenRhiNamespaceEnd
