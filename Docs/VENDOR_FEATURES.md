# Neural lighting and latency

The neural lighting target is GeForce RTX 50-series and newer and Radeon RX 9000-series and newer. The native provider resolves hardware/driver eligibility. `openrhiGetInterface` reports that an adapter was compiled; provider enumeration, successful context creation and submitted GPU execution are separate results.

Include `Extensions/OpenRHIVendorFeatures.h` after `OpenRHI.h`. The interfaces preserve the pinned SDK's native description ABI and use OpenRHI devices, command buffers and queues. Obtain native resources through `CoreInterface::Get*NativeObject`; native resource descriptions, layouts and usage follow the selected SDK. OpenRHI Validation checks its pointers and command-recording contract, while the SDK and graphics validation validate native description contents.

## Build and deployment

| Feature | Build option | SDK | Native integration |
|---|---|---|---|
| AMD Ray Regeneration | `OPENRHI_ENABLE_FFX_SDK` | FSR SDK 2.3.0, RR 1.2.0 | Windows x64 D3D12, `FidelityFXInterface` |
| AMD radiance cache | `OPENRHI_ENABLE_FFX_SDK` | FSR SDK 2.3.0, cache 0.9.0 preview | Windows x64 D3D12, `FidelityFXInterface` |
| NVIDIA neural radiance cache | `OPENRHI_ENABLE_NRC_SDK` | NRC 0.16, `7de30e4eb2a1a27118eb9a6acf5f715d757240c0` | Windows x64/ARM64 D3D12, `NeuralRadianceCacheInterface` |
| NVIDIA Reflex | `OPENRHI_ENABLE_NVAPI` on D3D12; native extension on Vulkan | Existing pinned NVAPI / `VK_NV_low_latency2` | `LowLatencyInterface` |
| AMD Anti-Lag 2 | `OPENRHI_ENABLE_ANTILAG2_SDK` | `390aa4a8c8655d0ae6e90079db2c85e103a96da3` | Windows x64 D3D12, `AntiLagInterface` |

CMake acquires unchanged provider inputs. NRC acquisition uses Git LFS for its binaries. `FETCHCONTENT_SOURCE_DIR_NRC`, `FETCHCONTENT_SOURCE_DIR_ANTILAG2` and `FETCHCONTENT_SOURCE_DIR_FSR` can select existing pinned SDK checkouts. FFX-enabled D3D12 builds stage the loader, upscaler, denoiser and radiance-cache DLLs. The maintained frame-generation gate also stages the signed framegeneration DLL; see [frame generation and presentation](FRAME_GENERATION.md). NRC-enabled builds stage the architecture-matching NRC and CUDA runtime DLLs. SDK preparation carries these runtimes and their original licenses. Native provider headers and shader helpers come from the matching SDK checkout, under its license. Bindings are generated from the matching OpenRHI public headers.

Prepare contexts and their pipelines before frame execution. OpenRHI's frame adapter calls use stack descriptors and cached objects. The caller owns resource creation, state transitions, UAV barriers between producers/consumers, queue submission, fences, overlap limits, retirement and native graphics-state rebinding after vendor dispatch. Keep devices alive until their contexts are destroyed. Calls on a context require external synchronization.

## AMD Ray Regeneration and radiance caching

Use `ffx_denoiser.h`, `ffx_radiancecache.h` and `dx12/ffx_api_dx12.h` from FSR SDK 2.3.0. OpenRHI owns the loader reference, native effect context, copied creation root and D3D12 backend description. Creation chains, callback data and other pointed-to creation objects remain alive until context destruction. The adapter inserts its persistent backend description ahead of the caller's `pNext` chain. The caller supplies effect descriptions and extensions; OpenRHI supplies the native device and command list.

```cpp
FidelityFXInterface ffx = {};
openrhiGetInterface(device, OPENRHI_INTERFACE(FidelityFXInterface), &ffx);

ffxCreateContextDescDenoiser native = {};
native.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_DENOISER;
native.version = FFX_DENOISER_VERSION;
native.maxRenderSize = {1920, 1080};
native.signalFlags = FFX_DENOISER_SIGNAL_INDIRECT_DIFFUSE
                   | FFX_DENOISER_SIGNAL_INDIRECT_SPECULAR;
FidelityFXDesc desc = {FidelityFXEffect::RAY_REGENERATION, &native};
FidelityFXContext* context = nullptr;
Result result = ffx.CreateFidelityFXContext(device, desc, context);
```

Ray Regeneration accepts all seven public signal descriptions: direct/indirect diffuse and specular, dominant-light visibility, ambient occlusion and specular occlusion. At least one lighting signal is required; AO and specular occlusion supplement that signal. Use signed linear depth, 3-component motion/depth delta, octahedral normals/roughness/material encoding, the SDK's albedo encoding, row-vector matrices, depth bounds, jitter, frame index and reset flags. Chain the selected signal descriptions and optional debug view to `ffxDispatchDescDenoiser`. Its `commandList` is supplied in a stack copy; caller descriptions are preserved. Checkerboard origins and half-width input packing follow the SDK contract.

Radiance-cache creation uses `ffxCreateContextDescRadianceCache`, `FFX_RADIANCECACHE_VERSION` and explicit inference/training capacities. Dispatch uses `ffxDispatchDescRadianceCache`, supporting inference, training, reset, counter clearing, learning-rate and weight-smoothing overrides. The debug dispatch is also exposed. The caller's path tracer writes two `CacheInput` buffers (11 floats per record: position, polar normal/view direction, albedo, roughness), two `CacheOutput` buffers (three floats per radiance record) and two uint32 counters. Counters must stay within the context capacities. A consumer pass composites predicted radiance with path throughput after the dispatch barrier.

`ConfigureFidelityFX` accepts the native configuration chain, including denoiser key/value controls. `QueryFidelityFX` accepts native query chains, including default values, memory usage and provider versions. A `FidelityFXDesc` with `nativeDesc=nullptr` creates a loader-owned query session for global configuration and eligibility enumeration. Keep that session alive while using returned provider-name pointers; release it with `DestroyFidelityFXContext`. The actual effect context remains a separate creation result. Use `CmdDispatchFidelityFX` with an open compute-capable OpenRHI command buffer outside rendering, then submit it explicitly. Drain its GPU work before destruction.

## NVIDIA NRC

Use `NrcD3d12.h`, `NrcCommon.h`, `NrcStructures.h` and the unchanged SDK shader helpers. `NeuralRadianceCacheDesc` references `nrc::GlobalSettings`, `nrc::ContextSettings`, optional `nrc::d3d12::Buffers` and an optional network configuration filename. Context creation initializes NRC once per process/native API, creates the device context and configures it. Live contexts share initialization only with identical global settings. Keep callback targets and the dependency-directory string alive across all such contexts. Final context destruction shuts the SDK down; partial creation failures unwind their references.

For caller-managed GPU memory, set `GlobalSettings::enableGPUMemoryAllocation=false`, query `nrc::BuffersAllocationInfo`, allocate native resources with its heap/sharing/UAV requirements, and pass `nrc::d3d12::Buffers` to creation or configuration. `GetNeuralRadianceCacheInitialBufferState` returns the SDK's native initial state. SDK-managed buffers are available as a borrowed `Buffers` pointer valid until reconfiguration/destruction. `ConfigureNeuralRadianceCache` handles resolution/model/reset changes between retired frames.

Frame order:

1. `CmdBeginNeuralRadianceCacheFrame` with `nrc::FrameSettings` on an open command buffer.
2. `GetNeuralRadianceCacheShaderConstants` fills caller-owned `NrcConstants`; upload and bind them with the SDK's query/training buffers.
3. The application's Slang path-tracing producers write query and training records through the SDK shader ABI.
4. Barrier those writes; call `CmdQueryAndTrainNeuralRadianceCache`. Optional training-loss output uses the SDK's diagnostic path.
5. Barrier the predicted radiance; call `CmdResolveNeuralRadianceCache` with a native D3D12 output resource, or composite the exposed query data in an application pass.
6. Submit the command lists; call `EndNeuralRadianceCacheFrame` with the same OpenRHI queue after submission. Signal the retirement fence after this end-frame call, so it also orders any SDK queue work; retire resources and destruction against that completion.

Duplicate begins, dispatch before begin, configuration during an active frame and duplicate ends return `INVALID_ARGUMENT`. Failed begins/ends preserve retry behavior. The adapter uses the complete native settings structures, including model reset, training controls and debug resolve modes. Application shaders remain `.slang`, compiled explicitly through `CMake/OpenRHISlang.cmake` with Slang 2026.20.

## Reflex and Anti-Lag 2

Reflex uses `LowLatencyInterface` on a swap chain created with `ALLOW_LOW_LATENCY`. Configure enabled/boost/minimum frame interval through `SetLatencySleepMode`. D3D12 also exposes submission-overlap optimization and `GetLatencySleepStatus`, including native driver state, sleep interval and frame-generation status. Call `LatencySleep` once before simulation/input sampling; emit simulation, input and render-submit markers with the frame's nonzero `presentId`. Use the same ID for tracked submission and presentation. Presentation markers are recorded by the swap-chain implementation.

Click-triggered flash, the Windows statistics ping and out-of-band render/present markers are public marker values. Windows applications register `PC_LATENCY_PING` with `RegisterWindowMessage` and pass received pings as the corresponding marker. `SetLatencyQueueRole` exposes native out-of-band queue registration for application/FG queues. Vulkan maps its distinct marker numbering explicitly; its native API provides render/present out-of-band roles. The sleep-status and minimum-queue-time options describe the D3D12 native API. `GetLatencyReport` returns the reported frame ID, CPU/GPU timings and the additional D3D12 camera, active-GPU, frame-generation and cross-adapter timing fields. Latency reductions require application-level measurement.

Anti-Lag 2 owns a persistent SDK context obtained with `CreateAntiLagContext`. Successful creation establishes native driver eligibility. Call `UpdateAntiLag(enabled,maxFPS)` once immediately before polling input. `MarkAntiLagEndOfRendering` follows submission of the main workload. `SetAntiLagFrameType` identifies rendered/interpolated frames immediately before their presentation. These calls cover the public SDK's frame-generation protocol.

For an FSR frame-generation swap chain, `AttachAntiLagSwapChain` accepts the native `IDXGISwapChain*` and publishes the SDK-required GUID/private-data record. It retains a COM reference, refreshes enable state on configuration changes and removes the record on detach/destruction. The caller retires presentation callbacks before detaching or destroying the context. Use the swap-chain hook or explicit per-frame type signaling according to the presentation provider's integration. `DestroyAntiLagContext` releases the driver interface and attachment after the caller has retired their use.

## Verification

`Tests/VendorFeatures.cpp` exercises provider eligibility, native all-signal RR dispatch/readback, AMD cache inference/training/readback/reset/counter controls and the Anti-Lag 2 public protocol on an RX 9070 XT. `Tests/NeuralRadianceCacheContract.cpp` compiles the production NRC adapter against deterministic SDK functions to verify failure cleanup, shared initialization, native handles, frame ordering and adapter allocation behavior. The latter is host-contract evidence. Record SDK/runtime hashes, native device identity and actual per-configuration results with each qualification run.

Primary contracts: [AMD Ray Regeneration](https://gpuopen.com/manuals/fsr_sdk/techniques/denoising/), [AMD radiance caching](https://gpuopen.com/manuals/fsr_sdk/techniques/radiance-cache/), [NVIDIA NRC SDK](https://github.com/NVIDIA-RTX/NRC/tree/7de30e4eb2a1a27118eb9a6acf5f715d757240c0), [NRC integration guide](https://github.com/NVIDIA-RTX/RTXGI/blob/main/Docs/NrcGuide.md), [Anti-Lag 2 SDK](https://github.com/GPUOpen-LibrariesAndSDKs/AntiLag2-SDK/tree/390aa4a8c8655d0ae6e90079db2c85e103a96da3), [Vulkan low-latency API](https://docs.vulkan.org/refpages/latest/refpages/source/VK_NV_low_latency2.html).
