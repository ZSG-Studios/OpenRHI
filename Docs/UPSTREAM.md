# Upstream integration checklist

Reviewed on 2026-10-19 against NVIDIA NRI `a48ef10184e3cb434faec7cda1ec76a9574c354d` through `8ee40ca5c2efc3c8c9c980dc6d66c881fbc5ec77`. OpenRHI started this integration at `aed05687ae71e6e8cf61f8a04d9dd50e9d10f80a`.

Checkmarks record incorporated source changes or an explicit platform disposition. The verification matrix below records build and execution coverage separately. The integration commit retains the upstream tip as a parent so Git's upstream-only count represents pending work.

- [x] [`9b45d77c`](https://github.com/NVIDIA-RTX/NRI/commit/9b45d77cb2f58a4802f1393160c3593388199426) — Initialize memory-heap fields. `MemoryHeap` now declares zero defaults for size, alignment and memory type. Its constructor retains the requested type. OpenRHI's existing constructor alignment initialization is preserved through these declaration defaults.
- [x] [`a7de5042`](https://github.com/NVIDIA-RTX/NRI/commit/a7de50420acbbc980d64ead4b57551eeb2c79630) — Update pinned dependencies: VMA `bfb2ca99e1e7e9684dc06546b552ee3eff4b10aa`, D3D12MA `b73737b0b4cde0b5ab80d685209cdc0d81cf878e`, and NGX `310.9.16` (`9981c6350dc58c0ce21f024fd65e31e851c31689`). The upstream Metal release tag resolves to OpenRHI's existing immutable `27c4382b7151d55a51692cdcb27aaa98752240de` pin, which remains the source URL. Android integration instructions and SDK packaging fixtures use the updated versions. Vendor inputs remain unmodified.
- [x] [`3d8ca250`](https://github.com/NVIDIA-RTX/NRI/commit/3d8ca2504ba9776f0dbf801d9a58d5cfeee6182e) — Add `TextureViewDesc::layout` to the C/C++ facade, shared layout derivation, Validation, D3D12 depth/stencil view flags, Vulkan image descriptors/rendering attachments, and Metal view metadata. Explicit layouts take precedence over derived defaults. The retained OpenRHI backends are D3D12, Vulkan and Metal; D3D11/WebGPU hunks are reviewed platform exclusions. See [migration guidance](MIGRATION.md#texture-view-layouts).
- [x] [`bc3ebe74`](https://github.com/NVIDIA-RTX/NRI/commit/bc3ebe7431a27eab953e059347998d8d53274108) — Calculate streamer row size, row count and slice pitch in compressed blocks. OpenRHI's copy-batch ownership remains intact. Related coverage exposed rectangular ASTC dimension rounding: shared dimension derivation now uses each axis's block size and division-based rounding, including non-power-of-two block sizes.
- [x] [`5ed8bb69`](https://github.com/NVIDIA-RTX/NRI/commit/5ed8bb69a62ad52862ba2bc9c9e23b4426beae38) — Complete Vulkan-to-OpenRHI BC, ETC2/EAC and ASTC format conversion. Public API tests round-trip all 52 exposed compressed formats and preserve the unsupported BC1 RGB-only mappings.
- [x] [`7915ecaa`](https://github.com/NVIDIA-RTX/NRI/commit/7915ecaa128e51536f10f2d849892696cdc9872e) — Preserve `HAS_BUFFER` during acceleration-structure creation/sizing conversion. Command recording rejects it in vertex, index, transform, AABB and micromap-index slots before unwrapping geometry buffers.
- [x] [`a4e562eb`](https://github.com/NVIDIA-RTX/NRI/commit/a4e562ebc1d45d7690f0491d06ef067975020096) — Bound the effective Vulkan device version by the created instance version and retain the format-feature-flags extension wiring. OpenRHI keeps its Vulkan 1.3 minimum. The loader-cap regression exercises a real Vulkan 1.4 Adreno device through a test loader advertising Vulkan 1.3 and verifies the backend selects 1.3.
- [x] [`81245bd2`](https://github.com/NVIDIA-RTX/NRI/commit/81245bd2332161dda14464573363c64b95695781) — Detect supported presentation scaling before validating swap-chain dimensions. Scaled presentation validates against `minScaledImageExtent` / `maxScaledImageExtent`; ordinary presentation uses the surface's image extents. OpenRHI checks the requested scaling and both gravity bits against the selected present mode's capabilities before attaching the scaling structure, following the [native scaling contract](https://docs.vulkan.org/refpages/latest/refpages/source/VkSwapchainPresentScalingCreateInfoKHR.html).
- [x] [`8ee40ca5`](https://github.com/NVIDIA-RTX/NRI/commit/8ee40ca5c2efc3c8c9c980dc6d66c881fbc5ec77) — Review WebGPU-specific surface acquisition, capability filtering and `NOT_READY` behavior. This is an intentional platform exclusion: OpenRHI retains its D3D12/Vulkan/Metal acquisition and result contract.

## Verification

| Configuration | Build coverage | Execution coverage |
|---|---|---|
| Windows x64, D3D12 + NONE | Debug and Release; MSVC 19.51, Windows SDK 10.0.28000, Agility 1.619.6, updated D3D12MA | 15/15 tests per configuration; RX 9070 XT resource/mesh tests and upstream layout, geometry-sentinel and BC upload/readback regressions |
| Windows optional providers | Debug and Release with FFX, NRC, Anti-Lag 2, NVAPI and updated NGX | 18 selected tests per configuration, including NGX lifecycle, NRC/Reflex contracts, public API and SDK/export checks |
| Windows ANF host contract | Debug and Release, NONE configuration, pinned ANF public headers | ANF adapter contract, shared texture-view contract and backend-aware export checks; adapter selection skips the software-only configuration |
| Windows static NONE | Debug and Release static library | 8 host/API/packaging tests executed per configuration; adapter selection skips the software-only configuration |
| Linux x64, WSL Ubuntu 24.04 | Debug and Release, Clang 23.1.3, Vulkan-Headers 1.4.365, Vulkan 1.3 runtime and updated VMA | 12 tests executed per configuration, including all 52 format round-trips, llvmpipe Vulkan regressions and the instance-version cap; 3 hardware-selection/resource/mesh cases skip the software-only environment |
| Android ARM64, API 33 target | Debug and Release, NDK 30.0.16248370, Vulkan + ANF, updated VMA | Adreno 840 / Android API 37: public C/C++ API, device policies, OpenRHI Validation off/on, depth/stencil views, all five sentinel slots, BC/ETC2/rectangular ASTC readback, capped Vulkan 1.3 on a 1.4 device, descriptor compute, indirect drawing and resource readback |

The compressed-streamer regression failed on the original OpenRHI binary because the next allocation used texel rows instead of block rows. The rectangular-ASTC host regression also failed before the dimension-rounding correction. Both pass after integration.

Android execution used the Adreno Vulkan driver build `089d643e2a`, native driver version `2150932499`. The test-only capped loader forwards the real native implementation while advertising Vulkan 1.3; Android runs preload it explicitly. Its binary stays in the generated test directory, separate from SDK runtimes.

OpenRHI Validation and reported correctness errors were checked during native regression runs. Native API validation was requested; Khronos layer activation was not separately confirmed. Apple compilation/execution, the Windows SDK 10.0.20348 non-Agility configuration, and live Vulkan presentation/scaling remain separate verification gates. Optional vendor GPU workloads and performance-policy promotion retain their own qualification gates.

Local generated build, test and device receipts use `_Tmp/upstream-*`; native binaries and shader outputs use `_Build/` and `_Bin/`. The reusable tests are `PublicApiTests.cpp`, `TextureViewContract.cpp`, `UpstreamRegression.cpp` and `VulkanInstanceCap.cpp` under `Tests/`. `Tests/AndroidDeviceLab/RunUpstreamRegression.ps1` deploys the native executables, test-only loader, shaders and licensed runtimes and records each device invocation.

Refresh and verify ancestry with:

```sh
git fetch upstream
git rev-list --left-right --count upstream/main...main
```

The first count is the number of upstream-only commits; the second is the number of OpenRHI-only commits.
