# MIRA integration: OpenRHI

MIRA (Modular Integrated Rendering Architecture) at `C:/MIRA` composes OpenRHI, UVRT, FACET, and future modules through their public interfaces. OpenRHI remains owned and implemented in `C:/MIRA/Modules/LocalSources/OpenRHI`. This guide records the OpenRHI side of that boundary.

## Lighting delivery priority

Working UVRT lighting has priority above other work. OpenRHI advances concrete backend, public SDK and pinned shader/compiler blockers to lighting correctness, setup speed and live resource/lifetime qualification. Unrelated feature expansion is deferred. MIRA's `C:/MIRA/Docs/plan.md` owns integrated sequencing; UVRT's `C:/MIRA/Modules/LocalSources/UVRT/plan.md` owns lighting algorithms and phase dependencies. The [lighting delivery gates](QUALIFICATION.md#lighting-delivery-priority-and-gates) define the OpenRHI contribution and required evidence.

Use independent agents for source/API blocker review and assign one owner to each implementation. MIRA owns the host pipeline/cache implementation and serializes native GPU windows across modules. OpenRHI reviews and fixes demonstrated native or public API defects in its own repository. Keep the current qualified SDK while the existing public contract satisfies the lighting path; propose an SDK change with its exact blocker, consumer impact and qualification gates when needed.

### Setup and pipeline lifetime

Measure source-factory publication, typed binding creation, shader loading, layout creation, `CreateComputePipeline`, first submission/completion and target creation separately. A combined setup span establishes total latency; attributing it to pipeline creation requires the isolated call measurement.

The public core interface supports `CreatePipelineCache`, `DestroyPipelineCache` and the two-call `GetPipelineCacheData` operation. Query `DeviceDesc::features.pipelineCache` and `pipelineCacheControl`, supply `ComputePipelineDesc::cache`, and use `FAIL_ON_CACHE_MISS` for a restored-hit control only when supported. Retry incompatible `OUT_OF_DATE` cache data with an empty cache once and report continued failure. Serialize cache-using creation and serialize cache data only after those creations complete. Native D3D12 pipeline libraries require external synchronization for concurrent loads of the same PSO; the backend's store lock does not cover the full load/create/store transaction. See Microsoft's [pipeline library contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device1-createpipelinelibrary).

MIRA can retain a same-device pipeline/layout bundle across compatible tiles, rebinding and resize. Identify it by shader binary, reflected entry/threadgroup metadata, complete layout and pipeline configuration. Bind current heaps, root constants and resource indices for every recording. Retain the layout and pipeline through the last referencing recording/submission; retire generation-owned views, publications and buffers independently. Recreate the bundle for device or shader/layout changes. Qualify cold creation, live bundle reuse and restored-cache creation separately, preserving current-generation output, native validation and final lifetime restoration evidence.

### Concurrent immutable pipeline preparation

`OpenRHI.h` declares `Create*` operations thread-safe; `Destroy*` and `Cmd*` operations require external synchronization. MIRA can prepare an immutable pipeline/layout bundle on a setup worker while another owner prepares independent CPU scene data. Retain the same device, shader bytes, creation descriptor arrays, layout and cache through the synchronous creation call, then publish the complete bundle through a synchronized handoff. Allocation and message callbacks must handle concurrent worker calls. Serialize the full shared-cache load/create/store transaction, snapshots and destruction, and keep independent scene preparation outside that lock. Shared registries, producer command recording/submission and resource destruction retain their owner synchronization.

Keep the window/message owner responsive while preparing and reject reentrant operations that wait on the same setup future or cache lock. Cancellation withdraws publication and drains the creation task; join workers before device teardown and retire published pipeline users before destruction. Measure isolated PSO time and complete scene/pipeline readiness separately: overlapping work can reduce total preparation latency but does not bound the synchronous native creation call.

For MIRA's interactive `Begin`/`Collect` path, the caller admits the immutable shader/profile, creates the layout and gives the bundle an exclusively owned cache before returning to the application loop. The worker calls public compute creation and optional cache probing, placing the pipeline, result/error and timings in private completion storage. Caller `Collect` checks readiness without waiting, joins the ready result, publishes the complete bundle, exports the cache and performs error handling/cleanup. Caller methods and final destruction remain serialized. An exclusively owned cache needs its worker/caller handoff sequenced through completion; shared cache objects still require full transaction serialization. Keep short cache-file admission/export locking separate from the potentially long native creation call.

The first displayed scene has qualified lighting. Startup admits a ready lighting pipeline and coherent current-scene dependencies before displaying scene content, with a numeric launch-to-first-lit-present deadline declared before measurement. Pending replacement preparation retains a qualified lit current-scene baseline while authoritative simulation, camera input, UI/options, FSR composition and presentation advance through their owner paths. Poll readiness before classifying counted frames. After successful collection, acquire current pose/material/AS/resource generations and reset temporal history once at activation. The immutable bundle carries shader/layout state; generation-bound descriptors and recording/publication leases belong to the current consuming passes.

Qualify actual lit rendered preparation-frame and source-frame progress, camera/UI changes and presented motion as well as HWND message response. Record launch-to-first-lit-present and actual presented-frame p95/p99/maximum intervals, deadline misses and consecutive stalls across startup, preparation, activation/reset and steady execution. Record caller `Begin`, readiness polling, `Collect`/cache export and final teardown separately from native PSO duration and counted lighting frames. Public creation thread safety permits this ownership arrangement; integrated CPU/driver contention and latency require the actual combined application receipt. The render-loop preparation candidate remains subject to those native gates until its owning run passes.

Persistent application cache lookup uses a shared local content-addressed location keyed by exact device/backend/driver identity, complete shader bytes and reflected entry/workgroup metadata, root/layout and pipeline configuration. Preserve a compatible caller-supplied import and validate native cache compatibility before reuse; a miss follows the declared bounded startup/preparation policy. MIRA owns the local cache location, file admission/export synchronization and deployment implementation. Qualify independent output-directory launches, exact-key hits, intentional shader/layout/driver mismatches, empty/corrupt caches and safe recovery with first-lit and presented-frame evidence together. Cache creation or restored-hit component success supplies its measured scope; integrated responsiveness remains an application gate.

The same [production readiness gate](QUALIFICATION.md#production-profile-readiness) applies before enabling every feature/profile, including asynchronous assets and replacement generations. Prepare from immutable inputs while authoritative input/simulation/animation, rendering and presentation advance; collect into the owning loop and publish only coherent ready dependencies. Retain a qualified current-scene baseline through preparation, cold/reset caches, activation, pressure and recovery. Link actual scene/presented generations to per-signal before/changed/recovered correctness, numeric response/service/fallback/deadline/memory/queue limits and combined D01-D14 receipts. Component preparation or creation receipts contribute their scope while all 154 required production cells remain open with zero passing cells at this checkpoint.

SDK94 exposes `ComputePipelineBits::NONE` and `FAIL_ON_CACHE_MISS`; the latter verifies a cache hit rather than accelerating a cold compile. Build-time specialization candidates use separately named pinned Slang outputs through `openrhi_compile_shader(DEFINES ...)` or constant wrappers. Preserve the declared shader/root/descriptor ABI, direct nonuniform heap access and reflected workgroup metadata, retain a matching dynamic fallback, and qualify output/current-scene correctness before paired setup/runtime measurements. Existing secondary-lighting correctness failures take priority over specialization experiments.

### SDK94 isolated V5 pipeline receipt

The 20 October 2026 SDK-only control created MIRA's exact 87496-byte V5 shader, SHA256 `fefd2c4c7726fccb4cd4135520a27483fa487e771b879c45d2eeb391a3bd9be1`, with the frozen 16-byte b0/space1 compute root, both directly indexed heaps, `SurfacePath` 64x1x1 metadata, null application cache and default pipeline flags/robustness. SDK94 Release DLL SHA256 was `94c0326816c019792ed67df9dcf09db0b328498da62c215f43a03e16d75cd751`. On RX 9070 XT UID 68069/0, native driver field `9007201352878040`, validation enabled and `nativeF64` admitted, the public `CreateComputePipeline` call returned `SUCCESS` in **88632.3114 ms**. The process completed within its original 180-second deadline at 93.6610466 seconds, with zero native errors, one warning and observed exit code zero after ordered begin/return/result/final markers and teardown.

The minimal runtime reported AMD AGS disabled because its optional DLL was absent. This receipt covers public PSO creation in that recorded configuration; null application cache leaves driver-managed cache state independent. The control issued no submission and retained the shader/layout/device through creation and ordered destruction. It did not reproduce MIRA's separate unexplained V5 early termination, whose failed application receipt remains open. Per-call creation time, application setup, runtime lighting correctness and paired performance promotion retain their separate gates.

Raw predeclared inputs, executable/source/runtime hashes, stdout/stderr, terminal receipt and offline postcheck are retained at `C:/MIRA/Modules/LocalSources/OpenRHI/_Tmp/ExactPsoControlV5/Runs/Native-20261020-Initial/`. PID 24212 was terminal and reaped; the native window was explicitly released before the offline postcheck. The earlier d077 control remains a separate frozen package.

## Scene-lighting ownership

UVRT owns scene illumination, BRDF evaluation, shadows, environment lighting, and lighting tone mapping. FACET and MIRA retain UVRT compatibility and composition code through public interfaces. MIRA owns frame scheduling and requested presentation settings. OpenRHI owns device creation, native backends, acceleration structures, public ray-query hardware interfaces, and vendor-provider adapters. Scene-lighting signal production and consumption belong to UVRT. This ownership contract governs the FACET-to-UVRT lighting migration and its integrated qualification.

## Source and build entry points

| Entry | Responsibility |
|---|---|
| `Include/OpenRHI.h`, `Include/OpenRHIDescs.h` | Public C/C++ facade, core function table, resource and capability descriptors; API revision 1 |
| `Include/Extensions/OpenRHIDeviceCreation.h` | Adapter enumeration, requirement evaluation, compatibility reports, device creation and destruction, immutable device profile |
| `Include/Extensions/` | Public swapchain, streaming, descriptor heap, ray tracing, mesh, video, upscaler, latency, vendor, and native wrapper interfaces |
| `Source/Creation/`, `Source/Shared/`, `Source/Validation/` | Creation, shared implementation, public-input validation |
| `Source/D3D12/`, `Source/VK/`, `Source/Metal/`, `Source/NONE/` | Native backends and explicit automated-test backend |
| `CMakeLists.txt` | Maintained CMake 3.30+ build, C++17, target `OpenRHI` |
| `CMake/OpenRHISlang.cmake`, `Include/OpenRHI.slang`, `Shaders/` | Pinned Slang 2026.20 build pipeline and shader ABI |
| `Scripts/Windows/`, `Scripts/Linux/`, `Scripts/MacOS/` | Deploy, Debug/Release build, SDK packaging |
| `Tests/`, `Scripts/Qualification/`, `Profiles/` | Contracts, native correctness, explicit measurement tools, policy data |

Windows uses D3D12; Linux and Android use Vulkan; Apple platforms use native Metal 4. Platform baselines are maintained in the [repository overview](../README.md). Generated products belong in `_Build/`, `_Bin/`, `_Tmp/`, and the packaged `_OpenRHI_SDK/`.

## Bindless handoff

MIRA owns the shared typed registry and lifetime protocol; FACET and UVRT consume GPU-selected indices. OpenRHI supplies public heap operations, native limits, descriptor encoding and Validation. Follow [the bindless contract](BINDLESS.md) for shader/root ABI, backend transports, generations, recorded/submitted retirement, UI/vendor rebinding and native migration gates.

## Live feature and generation handoff

Follow the [production live-update contract](QUALIFICATION.md#production-live-update-contract) aligned with UVRT plan v1.7. Each feature/public boundary declares authoritative live events, affected transport/regions, generation validity/invalidation, bounded scheduling, a valid current-scene fallback and numeric maximum service age before adoption. MIRA advances input/simulation/attachments, joins FACET exports with AS/resource publication, orders UVRT work and identifies the generation actually presented. OpenRHI supplies the explicit native resource, capability, queue and completion boundary.

Supported stationary entities remain eligible for next-tick movement and state changes. Carry player/body/equipment/attachment and reparenting updates, supported scale/deformation/topology changes, light/material edits and enabled/visibility transitions through current raster/AS/material/resource generations. New production features/content register against the full dynamic ledger, and unchanged-content shortcuts revalidate and invalidate automatically. Qualify appearance/removal of affected direct/shadow/diffuse/specular signals with stationary-camera moving-world and traversal cases. Admit the full simultaneous player/physics/animation/streaming/AS/transport/reconstruction/presentation working set with numeric CPU/GPU, memory/queue, deadline and response-age limits, current-scene fallback and bounded recovery. Preserve the 100% completion requirement and all unfinished production cells in the denominator.

MIRA defines the shared simulation tick/time and render interpolation sample consumed by raster/ray geometry, attachments, receivers and motion. Keep the matching previous render/presented sample and explicit reset on discontinuities. Consuming owners automatically propagate affected transport updates and validate unchanged-content classifications at use. Under coalescing, supersession and pressure, bound both oldest-pending dependency age and latest-visible freshness while retaining event provenance, starvation/stale-age observations and fallback duration.

Link the authoritative change frame/time to recorded/submitted/completed native work, consumed source revisions and the first correct presented frame. Declare first-changed-frame fallback error/duration and refresh bounds, including sustained changes, stationary-camera world motion, supported motion-rate boundaries and pressure/reset/recovery. Each phase exit regresses the combined pipeline and records correctness, latency, frame/pass/deadline and memory evidence together. The owning UVRT dynamic ledger retains all required cells, with 154 production cells open at this alignment checkpoint; module/native fixtures contribute their measured contracts.

MIRA and UVRT publish one shared numeric operating envelope per production profile across resolution/quality, simultaneous player/object/light/deformation counts, motion rates, streaming/content churn, response/image limits, CPU/GPU frame deadlines and queue/memory capacities. OpenRHI records native resource/AS preparation, submission and fence completion against those same limits and generation identities. Publish ready AS/geometry/material/resource generations coherently, retain the qualified current-scene baseline during bounded replacement preparation, and retire prior generations only after recording leases and final-use fences complete. Exercise the supported envelope boundary, pressure and recovery with the combined pipeline; every applicable production cell remains in the 100% coverage requirement.

## Public API handoff

Include `OpenRHI.h` first, then the required `Extensions/OpenRHI*.h` headers. C++ uses namespace `openrhi`; C uses `OpenRhi*` types and the same `openrhi*` exports. Build all consumers against matching revision-1 headers and binaries.

1. MIRA gathers module requirements and selects its platform backend. Use `openrhiGetPlatformRequirements`, `openrhiEnumerateAdapters`, and `openrhiQueryAdapterSupport` for explicit startup eligibility checks. The support query creates a temporary device, so schedule it during initialization.
2. Pass the chosen adapter, requirements, callbacks, queues, and validation settings to `openrhiCreateDevice`. Preserve its compatibility report on failure. Explicit selection stays on that adapter; automatic selection evaluates compatible adapters in discrete-GPU, available-budget, UID order.
3. Obtain `CoreInterface` through `openrhiGetInterface(device, OPENRHI_INTERFACE(...), ...)`, then query required extension tables by their declared interface names and sizes. Inspect the device description and immutable `openrhiGetDeviceProfile` before choosing optional paths.
4. MIRA supplies module adapters with caller-owned device/interface access and the resources, views, command buffers, layouts, states, and synchronization needed for each invocation. Define the exact UVRT/FACET handoff structures in MIRA with those module owners.
5. MIRA orders barriers, queue submissions, fence waits, presentation, and teardown. Complete outstanding GPU use before releasing shared resources; destroy dependent objects before `openrhiDestroyDevice`.

For native-object interoperation, use the public `WrapperD3D12Interface`, `WrapperVKInterface`, or `WrapperMetalInterface` and the creation entry points declared in their wrapper headers. Specify borrowed versus owned native objects and destruction order in each MIRA adapter contract. The extension headers define the actual available operations.

## Ownership and integration work

| Owner | Code and contracts |
|---|---|
| OpenRHI | C/C++ facade; backend objects and commands; Validation; native wrappers; device requirements and profiles; low-level extensions; shader ABI/compiler helper; dependency pins; SDK packaging and module tests |
| MIRA | Top-level Xmake orchestration invoking the maintained OpenRHI CMake build; provider configuration; module discovery; aggregate requirements; renderer/frame scheduling; shared resource contracts; cross-module barriers and lifetime; application runtime deployment; integrated tests and receipts |
| UVRT / FACET / future module | Their algorithms, kernels, module APIs, module tests, and implementation-specific state in their original checkouts |

The reviewed OpenRHI tree contains reusable RHI implementation and RHI-owned build/test tooling. MIRA integration work consists of a provider adapter, module handoff descriptors, frame scheduling, deployment composition, and integrated fixtures. Place those integration pieces in MIRA as they are implemented. Consume OpenRHI's shared implementation and shader helper through this module's maintained interfaces. Preserve module history, licenses, upstream remote, and vendor inputs here.

This module-side pass adds documentation and a durable ownership rule. The existing public API supplies the OpenRHI composition boundary; concrete MIRA adapter implementation is owned by `C:/MIRA`.

## Dependencies and deployment

OpenRHI CMake owns the dependency revisions and feature gates. Windows uses Windows SDK, Agility SDK 1.619.6, D3D12MemoryAllocator, and configured NVAPI/AGS/NVTX support. Vulkan builds fetch Vulkan-Headers and VulkanMemoryAllocator; Metal builds use pinned metal-cpp and Xcode 26+ with the Metal toolchain. Optional provider switches include `OPENRHI_ENABLE_NGX_SDK`, `OPENRHI_ENABLE_FFX_SDK`, `OPENRHI_ENABLE_XESS_SDK`, `OPENRHI_ENABLE_NRC_SDK`, `OPENRHI_ENABLE_ANTILAG2_SDK`, and Android `OPENRHI_ENABLE_ANF_SDK`.

MIRA records the OpenRHI commit, configuration, architecture, compiler, backend, enabled provider switches, shader hashes, and deployed runtime set. Consume dependencies through public interfaces and retain [third-party notices](../THIRD_PARTY_NOTICES.md) and licensed runtime payloads.

For Windows SDK consumption, expose `_OpenRHI_SDK/Include`, link the configuration-matching `_OpenRHI_SDK/Lib/<Debug|Release>/OpenRHI.lib`, and deploy `OpenRHI.dll`, `AgilitySDK/`, enabled vendor runtimes, and accompanying licenses from that configuration directory beside the executable. Include `OpenRHIAgilitySDK.h` in exactly one executable translation unit, as demonstrated by `Tests/BackendSmoke.cpp`, to export the Agility version/path. MIRA's Xmake provider can invoke the commands below and import these artifacts. The SDK currently supplies headers, libraries, runtime payloads, documentation, and CMake shader helpers; define the imported consumer target in MIRA.

A CMake consumer can compose the original checkout directly:

```cmake
set(OPENRHI_BUILD_TESTS OFF CACHE BOOL "OpenRHI module tests")
add_subdirectory("C:/MIRA/Modules/LocalSources/OpenRHI" "${CMAKE_BINARY_DIR}/OpenRHI")
target_link_libraries(MiraRenderer PRIVATE OpenRHI)
```

`MiraRenderer` denotes the target defined by the consumer. Configure OpenRHI feature switches before `add_subdirectory`; build shared/static consistently with the consumer's deployment and link policy. For production static linkage, preserve the native dependencies carried by the CMake target.

MIRA shader builds call `openrhi_compile_shader` from the maintained helper. Ship the selected DXIL, SPIR-V, or metallib together with reflected entry names, threadgroup dimensions, and task payload bytes. See [shader integration](SHADERS.md) for the native Metal 24-byte descriptor entry ABI and complete build examples.

## Concrete build and test commands

Run in PowerShell; prerequisites are CMake 3.30+, the Windows C++17 toolchain/SDK, and access to pinned build dependencies:

```powershell
Set-Location C:/MIRA/Modules/LocalSources/OpenRHI
cmake -S . -B _Build -DOPENRHI_BUILD_TESTS=ON -DOPENRHI_ENABLE_NONE_SUPPORT=ON
cmake --build _Build --config Release --parallel 8
cmake --build _Build --config Debug --parallel 8
ctest --test-dir _Build -C Release --output-on-failure
ctest --test-dir _Build -C Debug --output-on-failure
```

Package both configurations after successful builds:

```powershell
& C:/MIRA/Modules/LocalSources/OpenRHI/Scripts/Windows/3-PrepareSDK.bat
```

The packaging script moves an existing SDK to the Recycle Bin before generating its replacement. `OpenRHI.SDKPackaging` tests packaging in isolated retained fixtures; actual application deployment remains a MIRA integration gate.

On Linux and macOS, run the corresponding `Scripts/Linux/1-Deploy.sh` or `Scripts/MacOS/1-Deploy.sh` with `-DOPENRHI_BUILD_TESTS=ON -DOPENRHI_ENABLE_NONE_SUPPORT=ON`, then that platform's `2-Build.sh` and `3-PrepareSDK.sh`. Run CTest with the configured build directory/configuration. Android toolchain and device procedures are maintained in [qualification](QUALIFICATION.md) and [ANF integration](ANF.md).

For an explicit host-only test configuration, configure a separate directory with native/provider gates disabled:

```powershell
cmake -S C:/MIRA/Modules/LocalSources/OpenRHI -B C:/MIRA/Modules/LocalSources/OpenRHI/_Build/mira-none -DOPENRHI_BUILD_TESTS=ON -DOPENRHI_ENABLE_NONE_SUPPORT=ON -DOPENRHI_ENABLE_D3D12_SUPPORT=OFF -DOPENRHI_ENABLE_VK_SUPPORT=OFF -DOPENRHI_ENABLE_METAL_SUPPORT=OFF -DOPENRHI_ENABLE_NVTX_SUPPORT=OFF -DOPENRHI_ENABLE_IMGUI_EXTENSION=OFF -DOPENRHI_ENABLE_RMLUI_EXTENSION=OFF
cmake --build C:/MIRA/Modules/LocalSources/OpenRHI/_Build/mira-none --config Release --parallel 8
ctest --test-dir C:/MIRA/Modules/LocalSources/OpenRHI/_Build/mira-none -C Release --output-on-failure
```

## Qualification snapshot: 19 October 2026

Reviewed source baseline: `702dec56341d808a101bfb398c20f5c17e186f4d` plus this documentation change. The checkout was clean before the pass. The existing `_Build` configuration uses Visual Studio 18 2026, D3D12, NONE, Validation, Agility, NVAPI, AGS, and NVTX; optional neural/upscaler SDK gates are off.

| Gate | Evidence from this pass |
|---|---|
| Windows build | Incremental Debug and Release builds succeeded, including C facade and C++ consumers |
| Release tests | 15/15 passed; zero skips |
| Debug tests | 15/15 passed; zero skips |
| Native runtime | AMD Radeon RX 9070 XT, vendor `0x1002`, device `0x7550`, driver field `9007201352878040`; D3D12 smoke, upstream regression, resource/readback/descriptor compute/indirect draw/presentation, mesh, and adapter selection tests passed |
| Native validation | Resource and mesh receipts report `native_validation=true`, `validation_errors=0` |
| Contract/deployment checks | Public API, NONE, source boundary, policy, texture view, binary boundary, promotion-gate logic, SDK packaging fixtures, Slang contract, and vendor availability tests passed |
| Integration gate | See the MIRA-reported integration results below; full MIRA + UVRT + FACET composition and per-platform deployment require MIRA-owned receipts |
| Remaining platform gates | Linux/Android Vulkan, Apple Metal, Windows ARM64, optional providers, and the separate NONE-only command above were not executed in this pass |
| Performance gate | Paired/sustained performance qualification remains governed by [qualification gates](QUALIFICATION.md); passing promotion-gate logic verifies the gate implementation |

CTest diagnostics are generated under `_Build/Testing/Temporary/`; the Debug run log from this pass is retained under `_Tmp/MIRA/20261019-debug-ctest.log`. Record fresh per-device receipts when MIRA executes its integrated workload. OpenRHI module correctness here establishes the tested module boundary; integrated rendering quality and performance require that workload's measurements.

## MIRA-reported integration progress

The MIRA coordinating chat (`01a156fa-a7a5-7503-be4a-ef753f263388`) reported the following after the module-side qualification pass. These results were reported by the integration owner and were not rerun in this OpenRHI chat; exact workload receipts remain owned by MIRA.

- MIRA's native application owns the OpenRHI device through `mira::Device` and passes the device and graphics queue to FACET's reusable module. Device creation and teardown with validation passed on the RX 9070 XT.
- MIRA owns application window/input, presentation, statistics, scene data, and application tooling migrated from FACET. FACET implementation remains in `C:/MIRA/Modules/LocalSources/FACET`; OpenRHI implementation remains in `C:/MIRA/Modules/LocalSources/OpenRHI`.
- MIRA imports the module libraries and the maintained OpenRHI SDK. FACET's public-facade shared-lifetime test passed. The original and migrated 256-frame demo screenshots were reported byte-identical.

Keep application and composition changes in MIRA, FACET algorithm changes in FACET, and RHI changes in OpenRHI. Further integrated coverage records the exact MIRA/module revisions, runtime configuration, and retained test artifacts alongside those results.

## AMD presentation handoff

Maintained Windows x64 builds enable FSR, Anti-Lag 2 and `FrameGenerationInterface`. MIRA supplies FACET temporal guides, physical camera planes, separate HUDless scene/transparent UI, monotonically increasing frame ids, caller barriers/submissions and dynamic Present sync interval. Follow [the frame-generation contract](FRAME_GENERATION.md) for COMMON/GENERAL/ALL SDK interop, proxy ownership, disable/recreate, counters and deployment. Rebuild against the SDK public headers when adopting this interface and `DrawImguiDesc::writeAlpha`.
