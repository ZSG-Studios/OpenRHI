# MIRA integration: OpenRHI

MIRA (Modular Integrated Rendering Architecture) at `C:/MIRA` composes OpenRHI, UVRT, FACET, and future modules through their public interfaces. OpenRHI remains owned and implemented in `C:/OpenRHI`. This guide records the OpenRHI side of that boundary.

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
add_subdirectory("C:/OpenRHI" "${CMAKE_BINARY_DIR}/OpenRHI")
target_link_libraries(MiraRenderer PRIVATE OpenRHI)
```

`MiraRenderer` denotes the target defined by the consumer. Configure OpenRHI feature switches before `add_subdirectory`; build shared/static consistently with the consumer's deployment and link policy. For production static linkage, preserve the native dependencies carried by the CMake target.

MIRA shader builds call `openrhi_compile_shader` from the maintained helper. Ship the selected DXIL, SPIR-V, or metallib together with reflected entry names, threadgroup dimensions, and task payload bytes. See [shader integration](SHADERS.md) for the native Metal 24-byte descriptor entry ABI and complete build examples.

## Concrete build and test commands

Run in PowerShell; prerequisites are CMake 3.30+, the Windows C++17 toolchain/SDK, and access to pinned build dependencies:

```powershell
Set-Location C:/OpenRHI
cmake -S . -B _Build -DOPENRHI_BUILD_TESTS=ON -DOPENRHI_ENABLE_NONE_SUPPORT=ON
cmake --build _Build --config Release --parallel 8
cmake --build _Build --config Debug --parallel 8
ctest --test-dir _Build -C Release --output-on-failure
ctest --test-dir _Build -C Debug --output-on-failure
```

Package both configurations after successful builds:

```powershell
& C:/OpenRHI/Scripts/Windows/3-PrepareSDK.bat
```

The packaging script moves an existing SDK to the Recycle Bin before generating its replacement. `OpenRHI.SDKPackaging` tests packaging in isolated retained fixtures; actual application deployment remains a MIRA integration gate.

On Linux and macOS, run the corresponding `Scripts/Linux/1-Deploy.sh` or `Scripts/MacOS/1-Deploy.sh` with `-DOPENRHI_BUILD_TESTS=ON -DOPENRHI_ENABLE_NONE_SUPPORT=ON`, then that platform's `2-Build.sh` and `3-PrepareSDK.sh`. Run CTest with the configured build directory/configuration. Android toolchain and device procedures are maintained in [qualification](QUALIFICATION.md) and [ANF integration](ANF.md).

For an explicit host-only test configuration, configure a separate directory with native/provider gates disabled:

```powershell
cmake -S C:/OpenRHI -B C:/OpenRHI/_Build/mira-none -DOPENRHI_BUILD_TESTS=ON -DOPENRHI_ENABLE_NONE_SUPPORT=ON -DOPENRHI_ENABLE_D3D12_SUPPORT=OFF -DOPENRHI_ENABLE_VK_SUPPORT=OFF -DOPENRHI_ENABLE_METAL_SUPPORT=OFF -DOPENRHI_ENABLE_NVTX_SUPPORT=OFF -DOPENRHI_ENABLE_IMGUI_EXTENSION=OFF -DOPENRHI_ENABLE_RMLUI_EXTENSION=OFF
cmake --build C:/OpenRHI/_Build/mira-none --config Release --parallel 8
ctest --test-dir C:/OpenRHI/_Build/mira-none -C Release --output-on-failure
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
- MIRA owns application window/input, presentation, statistics, scene data, and application tooling migrated from FACET. FACET implementation remains in `C:/FACET`; OpenRHI implementation remains in `C:/OpenRHI`.
- MIRA imports the module libraries and the maintained OpenRHI SDK. FACET's public-facade shared-lifetime test passed. The original and migrated 256-frame demo screenshots were reported byte-identical.

Keep application and composition changes in MIRA, FACET algorithm changes in FACET, and RHI changes in OpenRHI. Further integrated coverage records the exact MIRA/module revisions, runtime configuration, and retained test artifacts alongside those results.

## AMD presentation handoff

Maintained Windows x64 builds enable FSR, Anti-Lag 2 and `FrameGenerationInterface`. MIRA supplies FACET temporal guides, physical camera planes, separate HUDless scene/transparent UI, monotonically increasing frame ids, caller barriers/submissions and dynamic Present sync interval. Follow [the frame-generation contract](FRAME_GENERATION.md) for COMMON/GENERAL/ALL SDK interop, proxy ownership, disable/recreate, counters and deployment. Rebuild against the SDK public headers when adopting this interface and `DrawImguiDesc::writeAlpha`.
