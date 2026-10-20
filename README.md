# OpenRHI

OpenRHI is a high-performance rendering hardware interface for D3D12, Vulkan, and native Metal 4. It provides an explicit C/C++ API, native resource access, user-controlled allocation and synchronization, device capability reports, and creation-time architecture profiles.

OpenRHI's first-party code and documentation use the [MIT license](LICENSE.txt). Upstream and source-derived attribution and third-party license terms are preserved in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and their original notices.

| Platform | Backend | Baseline |
|---|---|---|
| Windows 11 x64/ARM64 | D3D12 | Feature level 12_2, SM 6.6, Enhanced Barriers; stable Agility SDK 1.619.6 |
| Linux x64/ARM64 | Vulkan | Vulkan 1.3 minimum; 1.4 preferred |
| Android ARM64, API 33+ | Vulkan | Vulkan 1.3 minimum; 1.4 preferred |
| macOS/iOS/iPadOS 26+ | Metal 4 | Apple GPU family 9+ |
| Explicit automated-test configuration | NONE | `OPENRHI_ENABLE_NONE_SUPPORT=ON` |

Vulkan requires dynamic rendering, synchronization2, maintenance4, buffer device addresses, timeline semaphores, runtime descriptor arrays, partially bound descriptors, and sampled-image nonuniform indexing. Creation verifies the feature bits and returns structured diagnostics for missing requirements.

## Build

Use CMake 3.30+ and a C++17 toolchain. Windows builds use the Windows SDK and Agility SDK; Apple builds use Xcode 26+ with the Metal toolchain. Dependencies are fetched at pinned revisions through their public interfaces.

```sh
cmake -S . -B _Build -DOPENRHI_BUILD_TESTS=ON -DOPENRHI_ENABLE_NONE_SUPPORT=ON
cmake --build _Build --config Release --parallel
ctest --test-dir _Build -C Release --output-on-failure
```

Production builds enable their platform backend by default. Link the `OpenRHI` CMake target and include `OpenRHI.h`. Native wrapper entrypoints follow the supported platform. The platform scripts deploy, build, and package the SDK into `_OpenRHI_SDK`.

[Adreno Neural Fusion SR](Docs/ANF.md) covers the optional Android Vulkan upscaler, native requirements, resource setup, runtime packaging and device qualification.

## Language bindings

Zig bindings are maintained in `C:/Zig Engine/Bindings/Zig`. Rust bindings are maintained in `C:/Bevy/integrations/rust_openrhi`, with the `openrhi-sys` and `openrhi-rust` crates. Both consume the public OpenRHI C facade and qualified SDKs; their owning projects retain the binding sources, licenses and validation tooling.

## Shaders

Author shaders in Slang 2026.20 and compile them with `CMake/OpenRHISlang.cmake`. The build produces native DXIL, SPIR-V, or metallib plus reflection and matching pipeline metadata. ImGui, NIS, and Metal helper kernels use this same chain. [Shader integration and ABI](Docs/SHADERS.md) describes application setup and migration.

## Devices and qualification

Explicit adapter selection stays on the requested device. Automatic selection evaluates compatible devices in deterministic preference order. Device profiles separate actual capabilities from conservative upload, queue, and wave recommendations. Architecture recognition alone leaves tuning disabled.

The first measurement target is the AMD RX 9070 XT. The hardware matrix also includes NVIDIA Blackwell, Intel Xe2/Xe3, Apple M/A-series, Qualcomm Adreno, Arm Mali, and Samsung Xclipse. Exact per-device and driver receipts establish qualification.

- [API migration](Docs/MIGRATION.md)
- [MIRA module integration and ownership](Docs/mira-integration.md)
- [ImGui and RmlUi rendering integration](Docs/UI.md)
- [Module-owned rendering options and activation gates](Docs/OPTIONS.md)
- [Upstream integration checklist](Docs/UPSTREAM.md)
- [Qualification gates and measurement tools](Docs/QUALIFICATION.md)
- [Hardware targets and identity provenance](Profiles/targets.json)
- [CI builds](https://github.com/ZSG-Studios/OpenRHI/actions/workflows/build.yml)

OpenRHI retains core graphics/compute, descriptor heaps, streaming, ImGui rendering, low latency, mesh shaders, ray tracing, video, and optional upscaler interfaces. Device capability queries describe each backend's available features.

Neural lighting and latency adapters target RTX 50-series and Radeon RX 9000-series or newer: AMD Ray Regeneration and radiance caching use `FidelityFXInterface`, NVIDIA NRC uses `NeuralRadianceCacheInterface`, Reflex uses `LowLatencyInterface`, and Anti-Lag 2 uses `AntiLagInterface`. See [native contracts, build options and frame integration](Docs/VENDOR_FEATURES.md).

## Repository layout

| Directory | Contents |
|---|---|
| `Include/` | Public C/C++ API and extensions |
| `Source/` | Native backends, validation and shared implementation |
| `Shaders/` | Slang shader sources |
| `CMake/`, `Scripts/` | Build, packaging and qualification tools |
| `Tests/` | Maintained contract, platform and native integration tests |
| `Profiles/` | Device targets and policy data |
| `Docs/` | Integration, shaders, vendor features and qualification guides |
| `Resources/` | Project resources |

Builds generate `_Build/` and `_Bin/`; local logs, caches and qualification runs belong under `_Tmp/`. SDK packaging produces `_OpenRHI_SDK/`.

[MIRA — Modular Integrated Rendering Architecture](https://github.com/ZSG-Studios/MIRA) composes this repository as its `Modules/OpenRHI` Git submodule with [FACET](https://github.com/ZSG-Studios/FACET) and [UVRT](https://github.com/ZSG-Studios/UVRT). See [MIRA integration](Docs/mira-integration.md) for the public SDK and ownership boundary.
