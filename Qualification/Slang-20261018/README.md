# Slang native shader qualification, 2026-10-18

OpenRHI uses Slang 2026.20 for all authored shaders, with native DXIL, direct SPIR-V, and Metal 4 metallib outputs. These receipts cover the shader migration relative to `381b1973ce602429b6b7fb866a5a2dd8d12ba3a7`. The implementation tested by the complete native suites is `de120b996f708a208c45692665a02b0c9a509811`; `e0a68e95e8c591452b05073ded51ae2bacd7c114` removes a trailing blank line from `Shaders/NIS.slang`. The final receipt commit normalizes EmbedShader.cmake to CRLF and adds qualification records. Third-party provenance is recorded in [the notices](../../THIRD_PARTY_NOTICES.md).

## Results

| Configuration | Compatibility / correctness evidence | Result |
|---|---|---|
| Windows 11 build 26300, RX 9070 XT, D3D12 | MSVC 19.51.36260, Agility 1.619.6; Debug and Release | 10/10 tests passed in each configuration |
| macOS 27.0.1 (26A434), Apple M6, Metal 4 | Xcode 27.0 (27A266a), Apple Clang 21; Debug and Release | 9/9 tests passed in each configuration |
| Ubuntu 24.04 under WSL, Vulkan | GCC 13.3; Debug and Release | 6 passed, 3 GPU-dependent tests skipped in each configuration |
| Windows x64 SDK consumer | Compile its own Slang entry, load packaged library, create native compute pipeline | Passed |
| macOS ARM64 SDK consumer | Compile its own Slang entry through Metal assembly, load packaged library, create native compute pipeline | Passed |

The [ten-job CI matrix passed](https://github.com/ZSG-Studios/OpenRHI/actions/runs/39115929510) at `e0a68e95e8c591452b05073ded51ae2bacd7c114`, following the [complete implementation run](https://github.com/ZSG-Studios/OpenRHI/actions/runs/39114532957). [ci.json](ci.json) records exact jobs and source SHA. This includes Windows x64/ARM64/ARM64EC, Linux x64/ARM64, Android ARM64, macOS, iOS device ARM64, iOS simulator ARM64, and the source boundary. Cross-builds establish build coverage separately from physical device execution.

During the final Windows SDK refresh, the host stopped exposing the RX 9070 XT through DXGI and Windows device inventory. The remaining integrated/software adapters fail the required feature level; the refreshed consumer returned failure at device creation and resource/mesh tests skipped. `windows-final-adapters.jsonl` and `windows-final-host-state.txt` preserve this observation. Earlier full-suite and SDK-consumer passes above remain the physical-device evidence; a successful GPU rerun of the refreshed Windows package requires the RX 9070 XT to become available again. The refreshed package matches the final rebuilt library byte-for-byte. The manifests identify delivery build artifacts; earlier correctness receipts identify the tested source commit, rather than asserting identical linker output across rebuilds.

Native resource tests enable OpenRHI validation and native API validation. They verify upload/copy/readback, descriptor-array indexing, compute results, barriers, fence completion, indirect drawing and attachment-clear pixel results, NIS HDR/LDR pipeline creation and upscale readback, and task/mesh rendering with a four-byte task payload. Windows additionally executes presentation. Both resource receipts record zero validation errors. On Apple, `MTL_DEBUG_LAYER=1` and `MTL_SHADER_VALIDATION=1` were set. Adapter selection tests exercise explicit choices, rejected requirements, and deterministic selection on the available devices; Windows includes integrated and discrete AMD adapters.

Shader contracts check native binary signatures, entry names, reflected workgroup sizes, dependency tracking, generated descriptors, and Metal root-table offsets. Eleven selected application/test shaders are checked on D3D12/Metal and 23 on Vulkan. All 33 internal Metal shader groups compile and assemble into the native helper library. The manifests hash the selected artifacts, avoiding stale intermediates from earlier build configurations.

## Receipts and reproduction

- `windows-{release,debug}.log`, `macos-{release,debug}.log`, and `linux-{release,debug}.log`: complete final CTest summaries
- `windows-resources.json` and `macos-resources.json`: native device identity, workload settings, validation count, and raw samples from an additional Release correctness invocation
- `*-native.json`: SHA-256 of native libraries, selected shader binaries, reflection, entry points, and workgroup sizes
- `windows-os.json`, `windows-display.json`, and `macos-environment.txt`: observed OS, GPU, driver, and Apple toolchain identity
- `*-sdk-consumer.log` and [sdk-consumer](sdk-consumer): independent packaged-SDK pipeline test and its exact input sources
- `windows-final-subset.log`: final shader-contract pass and explicit GPU resource/mesh skips after the host adapter change
- `packages.json`: archive hashes, header/module inventory, and retired-pipeline audit results

Configure a native host build with tests, NONE, NIS, and ImGui enabled:

```sh
cmake -S . -B build -DOPENRHI_BUILD_TESTS=ON -DOPENRHI_ENABLE_NONE_SUPPORT=ON -DOPENRHI_ENABLE_NIS_SDK=ON -DOPENRHI_ENABLE_IMGUI_EXTENSION=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Use `CMAKE_BUILD_TYPE=Release` or `Debug` with single-configuration generators. Reconfigure/rebuild for both configurations. Compiler distribution pins and hashes live in `CMake/OpenRHISlang.cmake`; [SHADERS.md](../../SHADERS.md) documents native inputs and Metal resource layout. The resource command used eight measured iterations, the default 4 MiB staging workload, `--native-validation --nis`, descriptor-compute `--shader`, indirect `--vs/--ps`, and Windows `--present`. The separate mesh CTest uses two iterations with `--mesh`, the mesh binary, the task binary, and the fragment binary. Tests/CMakeLists.txt records complete arguments and validation environment.

For the independent SDK check, configure `sdk-consumer` with `-DSDK=/absolute/path/to/_OpenRHI_SDK`, build Release, and run `consumer`. Supply `OPENRHI_SLANG_ROOT` and, for Windows, `OPENRHI_SLANG_DXIL_ROOT` to reuse the pinned host tools. On Apple the packaged dylib is linked directly. A successful run prints `OPENRHI_SLANG_SDK_CONSUMER_PASS`.

## Qualification boundary and review

Compatibility and the exercised correctness subset passed on the recorded physical devices. Vulkan resources, mesh execution, and physical adapter selection skipped under WSL; Vulkan native GPU correctness remains a device-lab gate. Apple presentation, dedicated optional-feature workloads such as inline ray queries, every internal helper's runtime variant, and physical Android/iOS execution remain outside this receipt. ImGui shaders compile and their reflection contracts pass; interactive ImGui rendering is not measured here. The available Windows validation uses Agility; a separate Windows SDK 10.0.20348 non-Agility build was not run for this migration.

Performance status remains **unqualified**, with tuned production policies disabled. The short correctness-run samples were collected in the hosts' existing power configuration, without clock or thermal control. Apple GPU timestamp samples are zero in this workload and provide no GPU timing evidence. These samples do not establish speedups, sustained behavior, or promotion eligibility. Earlier paired RX 9070 XT results remain historical in their separate record.

Review traced shader compilation and reflection through ShaderDesc, Validation, D3D12/Vulkan/native Metal pipeline creation, dispatch dimensions, task payload storage, destruction, pipeline cache revision, feature reporting, exports, and SDK consumption. The final audit includes the complete migration diff and current working tree, source boundary checks, whitespace checks, and touched C/C++ CRLF verification. Available native validation and readback checks found no reachable implementation blockers. Remaining hardware coverage is recorded above. **Blockers: 0; useful non-blocking findings: 0. PASS** for the Slang migration within this coverage.
