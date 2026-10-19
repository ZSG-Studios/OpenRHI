# OpenRHI qualification

Qualification records belong to an exact device, OS, driver, backend, toolchain, commit, workload, and power configuration. Windows and Linux records remain separate for the same GPU.

## Gates

1. Platform: C/C++ builds, source audit, platform-specific native exports, dependency audit, and SDK packaging
2. Selection: explicit adapters, mixed UMA/discrete systems, deterministic ordering, unknown architectures, and rejected requirements
3. Correctness: upload/readback, descriptor indexing, barriers, compute, indirect drawing, queue/fence synchronization, presentation, and exercised optional features under native validation
4. Performance: five paired baseline/candidate runs; CPU/GPU median and p95, throughput, available memory budget, and zero correctness/validation failures
5. Promotion: at least 2% repeatable improvement, no p95 regression above 5% in another qualifying workload, and three 20-minute sustained runs for mobile
6. Evidence: retain raw samples, executable/shader/workload hashes, commit, profile revision, toolchain, host/firmware/driver identity, settings, and coverage

## Running checks

```sh
cmake -S . -B _Build -DOPENRHI_BUILD_TESTS=ON -DOPENRHI_ENABLE_NONE_SUPPORT=ON
cmake --build _Build --config Release --parallel
ctest --test-dir _Build -C Release --output-on-failure
```

Native GPU tests require suitable hardware and drivers. SDK/compiler availability controls shader-backed tests. Cross-compilation establishes build coverage; device receipts establish runtime coverage.

`OpenRHIQualification` exercises explicit buffer uploads, GPU copies, readback verification, timestamps, and synchronization. When supplied the generated descriptor-compute shader, it also verifies nonuniform descriptor-array indexing and compute output. `--variant device-upload` evaluates an explicitly requested GPU-upload candidate and reports a skip when that heap is unavailable. It does not change the production device profile.

```sh
python Scripts/Qualification/run.py --runner _Bin/Release/OpenRHIQualification.exe --api D3D12 --shader _Build/Tests/DescriptorCompute.dxil --output _Tmp/Qualification/local-run --power-state "describe measured power/clock settings" --toolchain "record compiler and SDK versions"
```

The runner performs five paired runs at 64 KiB, 4 MiB, and 16 MiB, alternates order between runs, and records raw CPU/GPU samples after eight warmup iterations. CPU samples cover CPU upload plus recording/submission/completion; GPU timestamps cover copies. Readback comparison runs outside the timed region on every iteration. Throughput is payload bytes divided by elapsed time, not aggregate physical bus traffic.

Use `--uid-low` and `--uid-high` to target an exact native adapter. `--mobile` extends the first three 4 MiB pairs to 20 minutes per variant; keep the device in the recorded power configuration throughout. The native runner also accepts `--seconds` for explicit sustained execution. Native validation is enabled separately for correctness runs with `--native-validation`; supply `--vs`, `--ps`, and `--present` to exercise indirect drawing and Windows presentation.

Profiles start unqualified and tuning stays disabled. Upload microbenchmarks alone do not promote a production policy; the remaining correctness, cross-workload, and sustained-execution gates must also pass. `Profiles/targets.json` records hardware targets and identity-source provenance.

## Evidence storage

Write local run outputs under `_Tmp/Qualification/<run-name>/`. Export accepted receipts with the release or consuming project, including native identity, source and artifact hashes, raw samples and validation results. Keep build coverage, native execution and policy promotion results separate.

Shader receipts include pinned Slang 2026.20, native output hashes, entry names and reflected workgroup dimensions. Re-run correctness after shader or binding-layout changes. See [shader integration](SHADERS.md).
