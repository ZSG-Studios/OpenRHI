# Rendering options for application menus

OpenRHI owns the UI-independent option descriptions and read-only public-interface queries in `Include/Extensions/OpenRHIOptions.hpp`. MIRA owns menu widgets, requested settings, application frame hooks, and active-context/dispatch state. Include `OpenRHI.h` first, then `Extensions/OpenRHIOptions.hpp`; this C++17 helper uses the public C/C++ facade and introduces no runtime dependency.

`GetOpenRHIRenderOptionDescs()` returns descriptions indexed by `RenderOption`: display name, declared interface name, build gate, required host input/hook bits, and whether a public execution path exists. `QueryOpenRHIRenderOptions(device, snapshot)` explicitly queries the current device's public interfaces and upscaler support reports at startup or settings refresh. It creates no contexts and changes no settings. Retain the snapshot alongside the application's own execution receipts.

Keep these states distinct:

1. The provider is included in the SDK build and deployed with its licensed runtimes.
2. The interface query succeeds on this device. For upscalers, `upscalerReportedSupported` additionally captures `IsUpscalerSupported`; provider/context creation is still required.
3. The application supplies the required resources, camera metadata, input/submission/presentation hooks and compatible formats.
4. Native context creation/configuration succeeds.
5. The requested path is actually used in frame execution. Only this state is an active feature. Retire its GPU/presentation work before reconfiguration/destruction.

The snapshot contains interface query results and upscaler support reports. It intentionally carries no application active/enabled state. For latency/vendor effects, interface availability requires a separate native eligibility/context query. Optional SDKs are controlled by the reported `OPENRHI_*` build gate; the UI-qualified default Windows SDK has FFX, NGX, XeSS, NRC, NIS and Anti-Lag 2 gates off, with NVAPI on.

Before live adoption, each option/provider boundary follows the [production live-update contract](QUALIFICATION.md#production-live-update-contract): authoritative setting/content events, affected transport, validity/history reset, bounded scheduling, current-scene fallback and numeric maximum service age. MIRA qualifies first changed-frame error/fallback duration, activation/reset/reconfiguration deadlines, continuous-change refresh and combined-pipeline regressions. Native eligibility/context/dispatch receipts retain their component scope alongside those application results.

`CmdDispatchUpscale` records through a void function-table entry. Track recorded, submitted and completed work separately and use native validation/readback receipts to qualify successful execution. A settings checkbox or an upscaler support query alone is not that evidence.

| Menu family | Public controls and execution contract | Required application integration |
|---|---|---|
| AMD FSR Super Resolution | `UpscalerInterface`: `IsUpscalerSupported(FSR)`, `CreateUpscaler`, `GetUpscalerProps`, `CmdDispatchUpscale`; request quality mode/output resolution, sharpness, reset history | Lower-resolution color, depth, motion vectors with declared scale/jitter convention, camera jitter, near/far planes, vertical FOV, frame time, pre-exposure, output resource; initialize/recreate between retired frames |
| AMD Anti-Lag 2 | `AntiLagInterface`: create context on user activation, `UpdateAntiLag(enabled,maxFPS)`, render-end marker, rendered/interpolated frame type | Update once immediately before input polling; mark after workload submission; set frame type before presentation; successful context creation establishes driver eligibility; attach a compatible native FG swapchain only when such a provider exists |
| AMD frame generation | `FrameGenerationInterface`: enumerate/create, acquire, configure, prepare, present, wait and query execution/DXGI counters | Actual depth/motion/camera inputs, caller submission/barriers, separate HUDless scene/UI and proxy-swapchain lifetime; see [frame generation](FRAME_GENERATION.md) |
| NVIDIA Reflex | `LowLatencyInterface`: low-latency/boost/minimum interval/queue policy, reported driver state and latency timings | `ALLOW_LOW_LATENCY` swapchain, sleep before input, consistent nonzero present IDs, simulation/input/submit markers, tracked queue submission and presentation; successful configuration precedes active reporting |
| NIS / MetalFX Spatial | `UpscalerInterface`: output resolution; NIS sharpening | Color and output resources, technique-specific formats and creation/dispatch; spatial techniques can be integrated with color-only application rendering |
| XeSS / DLSS SR / MetalFX Temporal / ANF | `UpscalerInterface`: quality/output resolution, history reset, provider-specific settings | Temporal color/depth/motion/camera producers; SDK, backend and hardware-specific eligibility; ANF device requirements are resolved before Vulkan device creation |
| DLSS RR / MetalFX Denoised / AMD Ray Regeneration | `UpscalerInterface` denoiser guides, or `FidelityFXInterface` native RR descriptions | Actual denoiser signals, motion/depth/camera conventions, public native provider descriptors, barriers, resource states and shader producers; the AMD interface's two effect types are RR and radiance cache |
| AMD radiance cache / NVIDIA NRC | `FidelityFXInterface` or `NeuralRadianceCacheInterface` context/configuration/dispatch/query | Path-tracer inference/training producers, counters/capacities, native SDK buffers/settings, barriers and submission protocol; provider availability remains separate from trained/used output |

See [vendor feature contracts](VENDOR_FEATURES.md), [upscaler/ANF requirements](ANF.md), and the public extension headers for exact descriptor fields and native ABI pins. FSR creation uses SDK 2.3.0 on Windows D3D12 and the maintained 1.1.4 Vulkan provider path; requesting FSR describes super resolution, while actual FSR 4 versus FSR 3 provider selection follows runtime eligibility.

MIRA can expose requested quality/sharpening, latency modes/FPS cap, device/profile/validation statistics, and its own presentation/camera controls now. Gate each execution control on the snapshot plus the actual host inputs/context result. Explain missing SDK/provider, device eligibility, depth/motion/camera producer or presentation hooks next to the control. Preserve requested settings separately so applying a setting can fail with its native result without claiming activation.

The NONE/Validation option snapshot and disabled-extension behavior are covered by `OpenRHI.RmlUi.Availability`; native UI tests cover the rendering boundary. Native feature execution uses the feature's own qualification tests and application workload receipts.
