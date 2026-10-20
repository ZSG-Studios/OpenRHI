# OpenRHI qualification

Qualification records belong to an exact device, OS, driver, backend, toolchain, commit, workload, and power configuration. Windows and Linux records remain separate for the same GPU.

## Gates

1. Platform: C/C++ builds, source audit, platform-specific native exports, dependency audit, and SDK packaging
2. Selection: explicit adapters, mixed UMA/discrete systems, deterministic ordering, unknown architectures, and rejected requirements
3. Correctness: upload/readback, descriptor indexing, barriers, compute, indirect drawing, queue/fence synchronization, presentation, and exercised optional features under native validation
4. Performance: five paired baseline/candidate runs; CPU/GPU median and p95, throughput, available memory budget, and zero correctness/validation failures
5. Promotion: at least 2% repeatable improvement, no p95 regression above 5% in another qualifying workload, and three 20-minute sustained runs for mobile
6. Evidence: retain raw samples, executable/shader/workload hashes, commit, profile revision, toolchain, host/firmware/driver identity, settings, and coverage

## Production live-update contract

Authority is UVRT `C:/UVRT/plan.md` version 1.5, sections 1 and 21, with case IDs and production status in `C:/UVRT/docs/dynamic-coverage.md`. MIRA's `Docs/plan.md` coordinates application phase exits. OpenRHI supplies native component evidence and public handoff requirements; the application/modules own the live simulation, lighting state and presented-generation ledger.

Before adopting a production feature or public integration boundary, record this contract for its supported profile:

| Contract field | Required declaration and evidence |
| --- | --- |
| Live authority | Owning input/simulation/content source, authoritative change frame/time, and source/pose/topology/placement/material/light/environment/resource revisions as applicable |
| Affected work | Receivers/casters, regions and direct/shadow/diffuse/specular transport dependencies affected by those events, including supported offscreen contributors |
| Validity | Compatible consumed generation, invalidation triggers, publication/withdrawal rules, and rejection or current-scene recovery for deliberately stale revisions |
| Bounded scheduling | Owner of discovery, queued work, supersession, refresh and recovery; numeric work/queue budgets and maximum service age measured from the authoritative event |
| Current-scene fallback | Current inputs used while work is pending, first changed-frame validity, numeric reference-error limit, maximum fallback duration and the first presented frame meeting the final limit |
| Continuous changes | Numeric refresh/service-age bounds while inputs keep changing or newer events supersede pending work; stationary-camera world motion and stop/restart/reversal controls |
| Supported motion | Declared motion-rate/displacement ranges, boundary cases, discontinuities and bounded recovery; moving player/world and independently moving receivers/casters |
| Interactive deadlines | Separate first-usable-frame, activation/empty-cache, reset/resize/resource replacement, recovery and steady-state deadlines; allowed total/consecutive misses and recovery limits |
| Cost and memory | CPU frame/submission/host time, GPU component/pass/AS time, end-to-end presented-frame distributions, working-set/peak/resident memory and live measurement duration |
| Exact evidence | Application/module/provider/SDK/compiler/shader/source identities, settings, native validation and lifetime results, predeclared limits/hash preceding the run, references, transition frames and receipt paths/hashes |

Limits are numeric and declared before measurement. An every-frame deadline profile permits zero misses across its declared interactive interval. Retain every transition frame separately from warmed distributions; report median/p95/p99/maximum times, deadline misses, queue/service age, image error and memory together. Production telemetry advances with input/simulation/animation and reports asynchronously; diagnostic readback/reference captures retain their own cost scope.

### Native and host responsibilities

| Boundary | OpenRHI component evidence | Consuming owner evidence |
| --- | --- | --- |
| Resource/descriptor publication | Public factory/capability admission, exact native shape/type, upload/copy/barrier operation and explicit fences | MIRA/module source revisions, valid GPU record publication, affected-region invalidation, recording/publication leases and bounded retirement/service age |
| Geometry and AS | Native build/refit/ray capability, resource access, command ordering and completion | FACET current geometry/material/motion export; MIRA matching AS/publication; UVRT current receiver/lighting dependencies and fallback |
| Provider/UI/presentation transitions | Interface/native eligibility, configuration/dispatch, native resource ownership, state effects and completion | MIRA requested/active state and scheduling, UI/content inputs, matching generations, current-scene fallback and first correct presented frame; UVRT temporal/lighting validity |
| Execution and recovery | Allocation-free frame execution, explicit queue dependencies, bounded declared capacities and native failure/validation results | Integrated CPU/GPU/frame/memory budgets, authoritative-to-presented latency, pressure/overflow/reset recovery and application support declaration |

Record the consuming owner's declared contract and receipts alongside existing public calls. Fence completion supplies native completion evidence. MIRA joins that evidence with consumed and presented scene generations to measure time-to-correct.

### Phase exits and coverage

Every phase exit and subsystem integration regresses prior passing dynamic cases with the combined pipeline enabled. Exercise overlapping player/object/deformation/light/material/streaming changes, stationary-camera world motion, offscreen contributors, supported motion-rate boundaries, pressure/recovery and stale-state controls. First changed frames must consume compatible current data or the declared valid current-scene fallback while bounded refresh converges; sustained changes must continue to receive service.

Accept each applicable system/case cell only when correctness, update latency/service age, deadlines and memory pass together for that declared profile. Retain passing/applicable counts and owner-reviewed applicability reasons; required unfinished systems remain open in the denominator. At this v1.5 alignment checkpoint the owning 11-system by 14-case ledger has **154 open production cells**. Keep bounded native fixtures and application checkpoints at their measured scope while the owning ledger accumulates complete production receipts.

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
