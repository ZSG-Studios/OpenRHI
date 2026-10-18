# OpenRHI naming and cleanup audit

Review basis: the complete owned source tree at `33bbea12d0fb8590418f708b4fd35438eb5690e6`, which matched remote main before review. Corrections: `73d15fe` for names, public API coverage and debugger views; `317a8a0` for native macOS Trash. Final review includes those commits and these evidence/documentation changes. Third-party provenance is recorded in [the notices](../../THIRD_PARTY_NOTICES.md).

## Findings corrected

1. **P2: stale debugger device layout**, `openrhi.natvis:38` and `:132`. The old visualizer reads names and descriptors at byte offsets 80/88. Adding DeviceProfileDesc storage to DeviceBase moves DeviceVal members, so inspecting a validated device can read the profile as a name pointer or device descriptor. Both C and C++ visualizers now address DeviceVal's named members. The signature check remains in place. XML parses successfully; interactive Visual Studio debugger evaluation was not run.
2. **P2: format API capitalization**, `Include/Extensions/OpenRHIHelper.h:86` and `Source/Creation/Creation.cpp:1146`. Two exports used an uppercase brand inside mixed-case function names. They now read `openrhiConvertOpenRHIFormatToDXGI` and `openrhiConvertOpenRHIFormatToVK`. Declarations, definitions, internal conversion helpers, and the optional upscaler call site agree. Migration documentation requires consumers to rebuild against the corrected spellings.
3. **P2: repeat SDK preparation stalls over SSH**, baseline `Scripts/Trash.sh:13`. Finder automation waited during the second macOS SDK cleanup. Native Foundation trashItem now handles the generated directory, checks its resolved repository path, rejects external symlinks, and verifies that the original target is gone. Both a disposable-directory test and repeat SDK packaging passed. The previous SDK remained recoverable in Trash.
4. **P3: extension descriptions use macro-style branding**, including `CMakeLists.txt:61`, `Include/OpenRHI.h:114`, and capability comments in OpenRHIDescs.h. They now use the declared interface names, such as HelperInterface, DescriptorHeapInterface, and LowLatencyInterface. Diagnostics use VideoInterface. Duplicate ImGui configuration in macOS CI was removed.

## Coverage and regression protection

The audit covers public C/C++ headers, namespaces, facade macros and types, exported functions, interface lookup strings, backend/private helpers, Slang sources, native wrapper boundaries, CMake targets/options/helpers, scripts, debugger XML, CI artifacts, SDK headers/libraries, and supported-platform routing. Retained NVIDIA licensing, attribution, upstream URLs and commit identity are preserved. Intel Arc product names remain hardware identities. Historical qualification records retain their measured results. Integration documentation uses the current API.

SourceBoundary now audits CMake helpers, shaders, tests, Profiles, CI, agent instructions, debugger support, and current documentation in addition to implementation and headers. It rejects old names in identifiers and filenames and incorrectly capitalized brand fragments. The current source guard permits upstream identity in third-party notices. Twelve positive/negative fixture cases passed; [boundary-cases.json](boundary-cases.json) records the historical fixture outcomes. Full original diagnostics are retained in the recovery archive.

The new PublicAPI test links a real C11 translation unit into a C++17 executable, exercises the renamed native format functions and format metadata, and checks short, namespaced and C-prefixed CoreInterface lookup using NONE. Windows export auditing additionally enforces the corrected format entrypoints. [export-audit.json](export-audit.json) compares every public declaration against each native library: exactly **22 expected exports on each platform**, with no missing or extra exports, and the matching D3D12, Vulkan or Metal creation wrapper. Raw symbol and dependency dumps are retained alongside it.

| Validation | Result |
|---|---|
| Windows D3D12 shared build, Debug / Release | Both built; 7/7 selected host tests passed in each |
| Linux Vulkan shared build, Debug / Release | Both built; 6/6 selected host tests passed in each |
| macOS native Metal shared build, Debug / Release | 10/10 tests passed in each, including resource and task/mesh execution under native validation |
| Windows static NONE build, Release | 5/5 selected tests passed |
| Independent Windows packaged SDK consumer | C11 names, symbols and native format conversion passed without device creation |
| Independent macOS packaged SDK consumer | Slang compilation and native compute-pipeline creation passed |
| Native macOS Trash and external-symlink rejection | Both passed; repeat SDK packaging passed |
| Source boundary fixture cases | 12/12 passed |

[The native CI matrix](https://github.com/ZSG-Studios/OpenRHI/actions/runs/39117925676) covers Windows x64/ARM64/ARM64EC, Linux x64/ARM64, Android ARM64, macOS, iOS device ARM64, iOS simulator ARM64 and source audits. `ci.json` records the exact source SHA and conclusions. SDK archives were refreshed with matching headers, native libraries, CMake shader helpers and debugger support. `packages.json` records hashes and inventory checks; previous local SDK archives were moved to the Recycle Bin. Prior Slang package receipts describe earlier artifacts.

Run `cmake -DROOT=. -P Tests/SourceBoundary.cmake` for source naming checks. Build with tests and NONE enabled, then run `ctest --test-dir <build> -C <config> -R 'PublicAPI|NONE|SourceBoundary|DevicePolicy|WindowsBinaryBoundary|PromotionGate|SlangContract' --output-on-failure` for host checks. The full native Metal suite uses the existing Tests/CMakeLists.txt validation environment. Public headers were edited manually; touched test C/C++ uses the repository formatter. Final whitespace and C/C++ CRLF checks pass.

## Review verdict and limits

All identified naming, debugger-layout and SSH-cleanup findings above are fixed. This is a naming/integration review of the current tree, not exhaustive qualification of every rendering algorithm or optional SDK. Current Windows native-device reruns remain unavailable while the RX 9070 XT is absent; Linux GPU execution, physical mobile execution, optional upscaler SDK builds, interactive debugger rendering, and the separate non-Agility Windows SDK build were not performed here. Existing performance promotion status remains unchanged. The WSL build emitted a host clock-skew warning; compilation completed and the recorded Linux host checks passed.

**Open blockers: 0; open useful non-blockers: 0. PASS** for the naming and cleanup audit within the recorded coverage.
