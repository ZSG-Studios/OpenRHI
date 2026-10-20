# OpenRHI qualification

Qualification records belong to an exact device, OS, driver, backend, toolchain, commit, workload, and power configuration. Windows and Linux records remain separate for the same GPU.

## Lighting delivery priority and gates

Working UVRT lighting has priority above other work. OpenRHI's immediate work supports lighting correctness, setup speed and live resource/lifetime qualification through its existing public SDK. Defer unrelated feature expansion. MIRA owns the integrated plan and native GPU window; UVRT owns lighting algorithms and phase exits. Use multiple agents for independent source/API investigation and review, with one implementation owner per change.

| Goal | Owning work and exit evidence |
| --- | --- |
| Identify a concrete blocker | Trace the actual MIRA/UVRT call through public API, capability admission, Validation, backend commands and retirement; record exact source/SDK/shader identities and distinguish defects from unmeasured behavior |
| Reduce lighting setup cost | MIRA separates factory/binding, shader loading, layout/PSO creation and first-completion timings; reuse compatible same-device pipeline/layout bundles and qualify persistent cache restore through public SDK capabilities |
| Preserve live lighting correctness | Match raster/material/pose/AS/lighting revisions at use; retain native validation, actual scene-color/reference results and source-to-presented generation evidence through wind, update and resize |
| Prove resource lifetime | Retain pipelines/layouts and all recorded/published resource versions through completion; qualify rebinding, replacement, reset, pressure and teardown with final descriptor/storage restoration |
| Change SDK only when required | Keep the current qualified SDK while its public path satisfies the workload; for a demonstrated blocker, qualify the smallest owning backend/Validation/public-contract change and its consuming integration before promotion |

Native measurements require MIRA's explicit serialized GPU window. Source inspection and CPU-only compiler/contract work may proceed independently. Preserve failed receipts and predeclared limits. Setup speed and warmed frame throughput have separate measurements; retain transition frames and measure actual setup/activation deadlines. Passing a bounded helper fixture contributes its scope to the production ledger; completion continues to require the owning UVRT/MIRA phase gates below.

CPU review on 20 October 2026 at source `0353e1787f03f559f9d440ebc1e605fc06e63a6a` found no demonstrated backend/compiler blocker in MIRA's current single-BLAS D3D12 lighting call path. Native setup timing, cache restore/reuse, current-generation color/reference results and live lifetime regressions remain consuming-owner gates. A separate batch-BLAS correction is deferred from this current-path work: `Source/D3D12/CommandBufferD3D12.hpp:2739` indexes `desc.geometries[i]` inside the inner `j` loop. Two descriptors containing one geometry each can read outside the second geometry array. The correction is `desc.geometries[j]`, paired with a two-descriptor regression before enabling that batch path. MIRA's reviewed `TriangleScene.cpp` calls pass one descriptor; this finding establishes neither the cause nor the cost of its current lighting setup. Review evidence here is source-only.

## Gates

1. Platform: C/C++ builds, source audit, platform-specific native exports, dependency audit, and SDK packaging
2. Selection: explicit adapters, mixed UMA/discrete systems, deterministic ordering, unknown architectures, and rejected requirements
3. Correctness: upload/readback, descriptor indexing, barriers, compute, indirect drawing, queue/fence synchronization, presentation, and exercised optional features under native validation
4. Performance: five paired baseline/candidate runs; CPU/GPU median and p95, throughput, available memory budget, and zero correctness/validation failures
5. Promotion: at least 2% repeatable improvement, no p95 regression above 5% in another qualifying workload, and three 20-minute sustained runs for mobile
6. Evidence: retain raw samples, executable/shader/workload hashes, commit, profile revision, toolchain, host/firmware/driver identity, settings, and coverage

## Production live-update contract

Authority is UVRT `C:/UVRT/plan.md` version 1.7, sections 1 and 21, including the runtime-content admission update published as `202ae8e7f6b0292fc97e0400504773f46815097e`, with case IDs and production status in `C:/UVRT/docs/dynamic-coverage.md`. MIRA's `Docs/plan.md` coordinates application phase exits. OpenRHI supplies native component evidence and public handoff requirements; the application/modules own the live simulation, lighting state and presented-generation ledger.

Treat every supported entity as eligible to change on the next simulation tick, including stationary players, equipment, attachments, objects, lights and cameras. Admit translation, rotation, supported scale, attachment/reparenting, enabled/visibility changes and the owning boundary's deformation, topology and resource changes. Register each production feature and supported content class against the complete responsibility matrix and D01-D14 cases before adoption. Revalidate unchanged-content optimizations at consumption and automatically leave them on relevant changes; qualify moving content with a stationary camera/player and during traversal.

Profile admission covers the simultaneous working set of player/physics/animation, moving receivers/casters, changing lights/materials, streaming, AS/metadata maintenance, transport, reconstruction and presentation. Predeclare CPU/GPU deadlines, memory/queue limits and maximum change-to-presented-response ages for that combined workload. Require each affected direct, shadow, diffuse and specular signal's appearance and removal to satisfy its correctness and temporal limits, with current-scene fallback and bounded recovery under pressure. OpenRHI's contribution is native capability/resource/AS ordering and completion evidence; MIRA and UVRT qualify live propagation and presented lighting.

Before adopting a production feature or public integration boundary, record this contract for its supported profile:

| Contract field | Required declaration and evidence |
| --- | --- |
| Live authority | Owning input/simulation/content source, authoritative change frame/time, shared simulation tick/time and render interpolation sample, matching previous-sample/reset identity, and source/pose/topology/placement/material/light/environment/resource revisions |
| Affected work | Receivers/casters, regions and direct/shadow/diffuse/specular transport dependencies affected by those events, including supported offscreen contributors |
| Validity | Compatible consumed generation, invalidation triggers, publication/withdrawal rules, and rejection or current-scene recovery for deliberately stale revisions |
| Bounded scheduling | Owner of automatic discovery, dependency invalidation, queued work, coalescing/supersession, refresh and recovery; numeric work/queue budgets plus oldest-pending dependency age and latest-visible freshness measured from authoritative events |
| Current-scene fallback | Current inputs used while work is pending, first changed-frame validity, numeric reference-error limit, maximum fallback duration and the first presented frame meeting the final limit |
| Continuous changes | Numeric refresh/service-age bounds while inputs keep changing or newer events supersede pending work; stationary-camera world motion and stop/restart/reversal controls |
| Supported motion | Declared motion-rate/displacement ranges, boundary cases, discontinuities and bounded recovery; moving player/world and independently moving receivers/casters |
| Interactive deadlines | Separate first-usable-frame, activation/empty-cache, reset/resize/resource replacement, recovery and steady-state deadlines; allowed total/consecutive misses and recovery limits |
| Cost and memory | CPU frame/submission/host time, GPU component/pass/AS time, end-to-end presented-frame distributions and full simultaneous working-set/peak/resident memory, including active, staged and retiring allocations/publications and queue/retirement pressure, plus live measurement duration |
| Exact evidence | Application/module/provider/SDK/compiler/shader/source identities, settings, declared CPU decode/transfer/filter reference policy, native validation and lifetime results, predeclared limits/hash preceding the run, references, transition frames and receipt paths/hashes |

Limits are numeric and declared before measurement. An every-frame deadline profile permits zero misses across its declared interactive interval. Retain every transition frame separately from warmed distributions; report median/p95/p99/maximum times, deadline misses, queue/service age, image error and memory together. Production telemetry advances with input/simulation/animation and reports asynchronously; diagnostic readback/reference captures retain their own cost scope.

MIRA defines one authoritative render interpolation sample and simulation tick/time. Raster geometry, ray geometry/AS, attachments, receivers and motion consume that sample. Previous-state motion refers to the corresponding previous rendered/presented sample, with an explicit reset at discontinuities. Retain those identities with source revisions and the presented frame so mixed-sample errors can be detected independently.

Consuming owners automatically discover authoritative changes and propagate affected primary visibility, motion, AS/receiver state, direct/shadow/diffuse/specular transport and reconstruction/history dependencies. Cover physics/hierarchy/attachments; skeletal/wind/procedural deformation; material/texture/emission edits; light changes/removal; spawn/despawn/topology/LOD/streaming/residency; environment/time-of-day; and lifecycle/quality transitions. Validate an unchanged-content classification at use and recover when its authoritative input changes.

During continuous changes, coalescing, supersession and pressure, measure both the oldest unresolved dependency and freshness of the latest visible state. Carry unresolved event provenance through queued-work replacement and record which generation satisfies it. Declare separate numeric age, starvation, fallback-duration and recovery bounds; include discovery, invalidation, uploads, AS maintenance, refresh and reconstruction in the CPU/GPU/memory budget. Admit the supported working set before exceeding its bounds.

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

Accept each applicable system/case cell only when correctness, update latency/service age, deadlines and memory pass together for that declared profile. Retain passing/applicable counts and owner-reviewed applicability reasons; required unfinished systems remain open in the denominator. At this v1.7 alignment checkpoint the owning 11-system by 14-case ledger has **154 open production cells**. The completion target remains 100% of applicable required systems and supported content through the combined production pipeline; new features retain their required coverage and phase regressions. Keep bounded native fixtures and application checkpoints at their measured scope while the owning ledger accumulates complete production receipts.

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
