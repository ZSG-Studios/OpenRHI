# Native platform gate

Source implementation: `2047241`. Comparison basis for the rename/profile work: `d4af360`. Third-party provenance is recorded in [the notices](../../THIRD_PARTY_NOTICES.md).

[The ten-job native CI matrix passed](https://github.com/ZSG-Studios/OpenRHI/actions/runs/39110589538). `ci-native-matrix.json` records the exact source SHA, jobs, steps, and conclusions.

| Configuration | Evidence |
|---|---|
| Windows x64 D3D12 | Debug/Release builds, host checks, native export/dependency audit, SDK artifact |
| Windows ARM64 / ARM64EC D3D12 | Debug/Release cross-builds, native binary audit, SDK artifacts |
| Linux x64 / ARM64 Vulkan | Native builds, NONE/policy/source/promotion host checks, SDK artifacts |
| Android ARM64 Vulkan, API 33 | Cross-build and SDK artifact |
| macOS Metal 4, OS 26 minimum | Debug/Release builds, host checks, SDK artifact |
| iOS device / simulator, ARM64, OS 26 minimum | Metal static-library builds and SDK artifacts |

Local Windows checks passed 8/8 in Debug and Release. Local Linux Vulkan builds passed in Debug and Release with 4/4 host-only checks in each configuration. A static NONE-only Windows build passed 4/4 selected host checks. The Windows Vulkan configuration test failed with the intended platform error. Logs are retained here and under the RX 9070 XT record.

The packaged Windows SDK was consumed by a separate C11 project that created a native device, queried its immutable profile, and destroyed the device. The SDK includes renamed public headers/libraries, migration and qualification documents, debugger metadata, Agility runtime files, and dependency-provided license files.

Build and packaging evidence establishes platform coverage. Physical-device runtime evidence currently comes from the local Windows RX 9070 XT record. Other device/driver/OS combinations remain unqualified; cross-queue ownership/overlap, optional-feature workloads, cross-workload performance comparisons, and mobile sustained qualification remain explicit device-lab gates.

Review traced creation, capability checks, explicit selection, rejection reports, validation wrapping, native ownership/destruction, exported interfaces, SDK names, and promotion rules. NVIDIA MIT licensing and upstream provenance are preserved. Available checks found zero implementation blockers. Hardware qualification limits are recorded above. **PASS** for the implemented platform/profile milestone.
